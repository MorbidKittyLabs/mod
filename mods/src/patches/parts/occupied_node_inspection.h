#pragma once

// Update runs even when input is focused, so old selections cannot survive a
// system/section change. HandleActionView consumes only an occupied-node cycle.
namespace occupied_node_inspection
{
void Update();
bool HandleActionView();
} // namespace occupied_node_inspection
