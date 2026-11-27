#include "occupied_node_inspection.h"
#include "occupied_node_cycle.h"
#include "errormsg.h"
#include "prime/Hub.h"
#include "prime/MiningObjectViewerWidget.h"
#include "prime/NavigationSectionManager.h"
#include "prime/PreScanTargetWidget.h"
#include "prime/navigation_binding.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace occupied_node_inspection
{
namespace
{
namespace binding = navigation_binding;
Cycle cycle;

double Now()
{
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

template <typename T> Il2CppObject* Object(T* object) { return reinterpret_cast<Il2CppObject*>(object); }

bool Visible(VisibilityController* visibility)
{
  return visibility && (visibility->_state == VisibilityState::Visible || visibility->_state == VisibilityState::Show);
}

bool ReadAddress(Il2CppObject* address, Address& value)
{
  return binding::Value(address, "get_Galaxy", "System.Int64", value.galaxy)
         && binding::Value(address, "get_System", "System.Int64", value.system)
         && binding::Value(address, "get_Instance", "System.Int32", value.instance) && value.system > 0;
}

bool CurrentAddress(NavigationManager* navigation, Address& value)
{
  auto* section = Hub::get_SectionManager();
  if (!section || section->CurrentSection != SectionID::Navigation_System) return false;
  auto* view = binding::Reference(Object(navigation), "get_ViewData", "Digit.Prime.Navigation.NavigationViewData");
  return ReadAddress(binding::Reference(view, "get_ViewingAddress", "Digit.PrimeServer.Models.NodeAddress"), value);
}

Il2CppObject* Poi(NavigationInteractionUIContext* context)
{
  return binding::ReferenceField(Object(context), "Poi", "Digit.Prime.Navigation.POI");
}

int64_t Id(Il2CppObject* object)
{
  int64_t id = 0;
  return binding::Value(object, "get_ID", "System.Int64", id) ? id : 0;
}

Il2CppObject* FindPoi(int64_t id)
{
  static auto helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Navigation", "POI");
  static auto* method = method_contract::Resolve(helper.get_cls(), "Get", true, "Digit.Prime.Navigation.POI",
                                                {"System.Int64"});
  Il2CppObject* result = nullptr;
  void* args[] = {&id};
  return binding::Invoke(method, nullptr, args, result) && Id(result) == id ? result : nullptr;
}

NavigationManager* Navigation()
{
  auto* sections = NavigationSectionManager::Instance();
  return sections ? sections->SNavigationManager : nullptr;
}

struct Selection {
  MiningObjectViewerWidget* mine = nullptr;
  PreScanTargetWidget* ship = nullptr;
  int64_t poi = 0, fleet = 0;
};

Selection Selected()
{
  // Prefer visible cards. Hidden pooled widgets are never restoration sources.
  for (auto* mine : ObjectFinder<MiningObjectViewerWidget>::GetAll()) {
    if (mine && Visible(mine->_visibilityController)) return {mine, nullptr, Id(Poi(mine->Context)), 0};
  }
  for (auto* ship : ObjectFinder<PreScanTargetWidget>::GetAll()) {
    if (!ship || !Visible(ship->_visibilityController)) continue;
    auto* buttons = ship->_scanEngageButtonsWidget;
    auto* battle = buttons ? buttons->Context : nullptr;
    auto* target = battle ? battle->TargetFleetDeployedData : nullptr;
    return {nullptr, ship, Id(Poi(ship->Context)), Id(Object(target))};
  }
  return {};
}

void Cancel(const char* reason)
{
  if (cycle.Active()) spdlog::info("[OccupiedNode] {} node={} occupant={}", reason, cycle.node, cycle.occupant);
  cycle.Clear();
}
} // namespace

void Update()
{
  if (!cycle.Active()) return;
  Address address;
  auto* navigation = Navigation();
  const auto selected = Selected();
  if (!navigation || !CurrentAddress(navigation, address) || !cycle.Matches(address, selected.poi, Now())) {
    Cancel("cancelled: section, selection or binding timeout");
    return;
  }
  if (selected.ship && selected.fleet > 0 && selected.fleet != cycle.occupant) {
    Cancel("cancelled: displayed fleet changed");
    return;
  }
  if (cycle.phase == Cycle::Phase::AwaitNode && selected.mine && selected.poi == cycle.node) {
    // Selection success alone is insufficient; wait for the native node card.
    Cancel("restored original node");
    return;
  }
  if (cycle.phase == Cycle::Phase::AwaitLoot && selected.ship && selected.poi == cycle.occupant
      && selected.fleet == cycle.occupant) {
    auto* rewards = selected.ship->_rewardsButtonWidget;
    auto* controller = rewards ? rewards->_rewardsController : nullptr;
    if (!controller) return;
    if (!Visible(controller)) controller->Show(true);
    if (Visible(controller)) {
      cycle.phase = Cycle::Phase::Loot;
      spdlog::info("[OccupiedNode] loot node={} occupant={}", cycle.node, cycle.occupant);
    }
  }
}

bool HandleActionView()
{
  Update();
  auto* navigation = Navigation();
  Address address;
  if (!navigation || !CurrentAddress(navigation, address)) return false;
  const auto selected = Selected();
  if (cycle.Pending()) return true;
  if (selected.mine) {
    Cancel("new node selection");
    auto* mine = selected.mine;
    if (mine->_occupiedState != OccupiedState::OtherPlayerOccupied) return false;
    auto* buttons = mine->_scanEngageButtonsWidget;
    auto* battle = buttons ? buttons->Context : nullptr;
    auto* target = battle ? battle->TargetFleetDeployedData : nullptr;
    DeployedFleetType fleet_type{};
    auto* poi = Poi(mine->Context);
    Address node_address;
    auto* poi_address = binding::Reference(poi, "get_Address", "Digit.PrimeServer.Models.NodeAddress");
    const auto fleet_id = Id(Object(target));
    if (!target || !target->TryGetFleetType(fleet_type) || fleet_type != DeployedFleetType::Player
        || !ReadAddress(poi_address, node_address) || node_address != address
        || !cycle.Begin(address, selected.poi, fleet_id, Now())) {
      spdlog::warn("[OccupiedNode] cannot capture original node identity node={} occupant={}",
                   selected.poi, fleet_id);
      return true;
    }
    if (!navigation->CanSelectPoi(fleet_id) || !navigation->SelectPoi(fleet_id)) {
      Cancel("occupant selection failed");
      return true;
    }
    spdlog::info("[OccupiedNode] selected occupant node={} occupant={}", cycle.node, cycle.occupant);
    return true;
  }
  if (!cycle.Active()) return false;
  if (cycle.Pending()) return true;
  if (!selected.ship || selected.fleet != cycle.occupant) {
    Cancel("target no longer matches");
    return false;
  }
  if (cycle.phase == Cycle::Phase::Loot) {
    auto* rewards = selected.ship->_rewardsButtonWidget;
    auto* controller = rewards ? rewards->_rewardsController : nullptr;
    if (!controller) { Cancel("rewards unavailable"); return true; }
    controller->Hide();
    cycle.phase = Cycle::Phase::Strength;
    spdlog::info("[OccupiedNode] strength node={} occupant={}", cycle.node, cycle.occupant);
    return true;
  }
  auto* original = FindPoi(cycle.node);
  Address original_address;
  auto* poi_address = binding::Reference(original, "get_Address", "Digit.PrimeServer.Models.NodeAddress");
  if (!ReadAddress(poi_address, original_address) || original_address != address
      || !navigation->CanSelectPoi(cycle.node) || !navigation->SelectPoi(cycle.node)) {
    // Consume this key without toggling back to loot or claiming restoration.
    spdlog::warn("[OccupiedNode] restoration failed node={} occupant={}; select the node again",
                 cycle.node, cycle.occupant);
    return true;
  }
  cycle.Restoring(Now());
  return true;
}
} // namespace occupied_node_inspection
