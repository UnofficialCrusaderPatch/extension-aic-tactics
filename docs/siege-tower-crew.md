# AI siege tower crews

Improved Siegeengines owns climbing, seats and docking. Its public
`registerTowerCrewPolicy`, `isClimbingReady`, `getFreeSeats` and `boardUnit`
(`rq/improved-siegeengines` `21a32cb` and `781cb3d`) run one policy from the
existing rider update before proximity boarding. AIC Tactics registers that
policy only when the module exposes `registerTowerCrewPolicy`. Otherwise,
including with Improved Siegeengines 1.0.31 from the Store, standard boarding
stays unchanged.

The policy sees the wall order from either the native order path or Improved
Siegeengines' `sendToWall`; moving towers have wall targeting `0x17` and order
state `0x65`. For characters with crew assignment enabled, it reserves seats of
siege-wave towers and, every 32 ticks, boards the troops of the AI's siege-defence
groups (slots 190-191) round-robin across the wall-bound towers. A false return keeps the proximity boarder. Registration
supports either module enable order.

OpenSHC's `Tribe.hpp` identifies the wave field and `Unit.hpp` the unit-to-tribe
ID and UID. The existing siege construction resolver decodes the wave offset,
including the Extreme tribe layout. The policy compares each tower's live tribe
and wave with the AI's current attack wave, and each rider's tribe and UID with
the siege-defence slots, as the native dispatcher does. The per-AI value is stored in the
existing configuration record's last word and takes part in the existing save
and multiplayer identity; an absent value uses the module setting
`siegeTowerCrew` (default off). The callback adds no game patch site.
