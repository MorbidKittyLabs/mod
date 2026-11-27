#include "patches/parts/occupied_node_cycle.h"
#include <iostream>
#include <stdexcept>

using namespace occupied_node_inspection;
void Require(bool condition) { if (!condition) throw std::runtime_error("occupied node regression"); }

int main()
{
  Cycle cycle;
  const Address origin{4, 5000000011LL, 1};
  const int64_t node = 9000000011LL, fleet = 8000000022LL;
  Require(!cycle.Begin(origin, node, node, 0));
  Require(!cycle.Active());
  Require(cycle.Begin(origin, node, fleet, 10));
  Require(cycle.Matches(origin, node, 10.1));
  Require(cycle.Matches(origin, 0, 10.2)); // native binding gap
  Require(cycle.Matches(origin, fleet, 10.3));
  Require(!cycle.Matches(origin, fleet + 1, 10.3)); // unrelated/replacement fleet
  Require(!cycle.Matches({4, origin.system + 1, 1}, fleet, 10.3));
  Require(!cycle.Matches({4, origin.system, 2}, fleet, 10.3)); // same system, other instance
  Require(!cycle.Matches({5, origin.system, 1}, fleet, 10.3));
  Require(!cycle.Matches(origin, fleet, 12)); // exact deadline
  cycle.phase = Cycle::Phase::Loot;
  Require(cycle.Matches(origin, fleet, 100)); // no inspection duration limit
  Require(!cycle.Matches(origin, node, 100));
  Require(!cycle.Matches(origin, 0, 100)); // dismissed selection
  cycle.phase = Cycle::Phase::Strength;
  Require(cycle.Matches(origin, fleet, 100));
  cycle.Restoring(100);
  Require(cycle.Active() && cycle.Pending()); // true SelectPoi is not binding success
  Require(cycle.node == node && cycle.occupant == fleet);
  Require(cycle.Matches(origin, node, 101));
  Require(!cycle.Matches(origin, node, 102)); // restoration timeout
  cycle.Clear();
  Require(!cycle.Active() && cycle.node == 0 && cycle.occupant == 0);
  Require(cycle.Begin(origin, node, fleet + 1, 105)); // fresh occupant on next cycle
  Require(cycle.occupant == fleet + 1);
  std::cout << "Occupied node identity, selection, instance and timeout regressions passed\n";
}
