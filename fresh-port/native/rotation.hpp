#pragma once
namespace aura {
// Writes direct rotation and the recovered EnTT yaw slot, when present.
void injectRotation(void* actor,float pitch,float yaw);
}
