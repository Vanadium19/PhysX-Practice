#include "BilliardsEventCallback.h"
#include "BilliardsGame.h"

void BilliardsEventCallback::onTrigger(physx::PxTriggerPair* pairs, uint32_t count) {
	extern BilliardsGame* game_;
	for (uint32_t i = 0; i < count; i++) {
		const physx::PxTriggerPair& pair = pairs[i];
		switch (pair.status) {
		case physx::PxPairFlag::eNOTIFY_TOUCH_FOUND:
			game_->RemoveBall(pair.otherActor);
			break;
		default:
			break;
		}
	}
}