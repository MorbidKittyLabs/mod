#pragma once

#include <cstdint>

namespace occupied_node_inspection
{
struct Address {
  int64_t galaxy = 0, system = 0;
  int32_t instance = 0;
  bool operator==(const Address&) const = default;
};

// Only identities survive a frame. In particular, pooled viewers and contexts
// are never kept alive or reused to restore the mining presentation.
struct Cycle {
  enum class Phase { Idle, AwaitLoot, Loot, Strength, AwaitNode };
  Phase phase = Phase::Idle;
  Address address;
  int64_t node = 0, occupant = 0;
  double deadline = 0;

  bool Active() const { return phase != Phase::Idle; }
  bool Pending() const { return phase == Phase::AwaitLoot || phase == Phase::AwaitNode; }
  void Clear() { *this = {}; }

  bool Begin(Address origin, int64_t node_id, int64_t fleet_id, double now)
  {
    Clear();
    if (origin.system <= 0 || node_id <= 0 || fleet_id <= 0 || node_id == fleet_id) return false;
    address = origin;
    node = node_id;
    occupant = fleet_id;
    phase = Phase::AwaitLoot;
    deadline = now + 2;
    return true;
  }

  bool Matches(Address current, int64_t selected, double now) const
  {
    if (!Active() || current != address || (Pending() && now >= deadline)) return false;
    if (phase == Phase::AwaitLoot) return selected == 0 || selected == node || selected == occupant;
    if (phase == Phase::AwaitNode) return selected == 0 || selected == node || selected == occupant;
    return selected == occupant;
  }

  void Restoring(double now)
  {
    phase = Phase::AwaitNode;
    deadline = now + 2;
  }
};
} // namespace occupied_node_inspection
