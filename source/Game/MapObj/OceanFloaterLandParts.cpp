#include "Game/MapObj/OceanFloaterLandParts.h"
#include "Game/LiveActor/LodCtrl.h"
#include "Game/LiveActor/Spine.h"
#include "Game/MapObj/MapParts/MapPartsRailMover.h"
#include "Game/Util.h"

OceanFloaterLandParts::OceanFloaterLandParts(const char* pName)
    : LiveActor(pName), mRailMover(), mLodCtrl(), mRailEndPos(0.0f), mObjectName(), mIsMoving(), mIsRegisteredDemo(), mAlreadyDoneFlag(-1) {
}

namespace NrvOceanFloaterLandParts {
    FULL_NERVE(HostTypeWait, OceanFloaterLandParts, Wait);
    FULL_NERVE(HostTypeMove, OceanFloaterLandParts, Move);
    FULL_NERVE(HostTypeDone, OceanFloaterLandParts, Done);
};  // namespace NrvOceanFloaterLandParts

void OceanFloaterLandParts::init(const JMapInfoIter& rIter) {
    MR::getObjectName(&mObjectName, rIter);
    MR::initDefaultPos(this, rIter);
    initModelManagerWithAnm(mObjectName, nullptr, nullptr, false);
    initEffectKeeper(0, nullptr, false);

    //if (MR::isEqualString(mObjectName, "OceanFloaterChurch")) {
    //    MR::setEffectHostSRT(this, "Rise", &mRailEndPos, nullptr, nullptr);
    //}

    initSound(4, false, nullptr, TVec3f(0.0f));
    initHitSensor(1);

    MR::initCollisionParts(this, mObjectName, MR::addBodyMessageSensorMapObj(this), nullptr);
    MR::connectToSceneCollisionMapObj(this);
    MR::setClippingTypeSphereContainsModelBoundingBox(this, 100.0f);
    initLodCtrl(rIter);

    const char* pName = mObjectName;
    if (MR::isExistAnim(this, pName)) {
        MR::tryStartAllAnim(this, pName);
    }

    bool notConnected = !MR::isConnectedWithRail(rIter);
    if (notConnected) {
        initNerve(&NrvOceanFloaterLandParts::HostTypeDone::sInstance, 0);
    } else {
        initMoveType(rIter);
    }

    if (MR::isEqualString("OceanFloaterTypeU", mObjectName)) {

        if (MR::setupAlreadyDoneFlag(&mAlreadyDoneFlag, "フローター浮上", rIter, false)) {
            MR::moveCoordToEndPos(this);
            MR::moveTransToCurrentRailPos(this);

            if (MR::isValidSwitchA(this)) {
                MR::onSwitchA(this);
            }

            setNerve(&NrvOceanFloaterLandParts::HostTypeDone::sInstance);
        }
    }

    makeActorAppeared();
}

void OceanFloaterLandParts::makeActorAppeared() {
    LiveActor::makeActorAppeared();

    if (mLodCtrl != nullptr) {
        mLodCtrl->validate();
    }
}

void OceanFloaterLandParts::makeActorDead() {
    if (mLodCtrl != nullptr) {
        mLodCtrl->invalidate();
    }
    LiveActor::makeActorDead();
}

void OceanFloaterLandParts::initMoveType(const JMapInfoIter& rIter) {
    initRailMoveFunction(rIter);
    MR::calcRailEndPos(&mRailEndPos, this);
    initNerve(&NrvOceanFloaterLandParts::HostTypeWait::sInstance, 0);
    if (MR::tryRegisterDemoCast(this, rIter)) {
        MR::registerDemoActionNerve(this,&NrvOceanFloaterLandParts::HostTypeMove::sInstance, nullptr);
        mIsRegisteredDemo = true;
    }
    MR::useStageSwitchWriteA(this, rIter);
}

void OceanFloaterLandParts::initRailMoveFunction(const JMapInfoIter& rIter) {
    if (MR::isConnectedWithRail(rIter)) {
        initRailRider(rIter);

        mRailMover = new MapPartsRailMover(this);
        mRailMover->init(rIter);
        mRailMover->start();
    }
}

void OceanFloaterLandParts::initLodCtrl(const JMapInfoIter& rIter) {
    mLodCtrl = MR::createLodCtrlPlanet(this, rIter, -1.0f, -1);
}

void OceanFloaterLandParts::control() {
    if (mLodCtrl != nullptr) {
        mLodCtrl->update();
    }
}

bool OceanFloaterLandParts::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (msg == ACTMES_MAPPARTS_DISAPPEAR_WITH_BLINK && isNerve(&NrvOceanFloaterLandParts::HostTypeMove::sInstance)) {
        kill();
        return true;
    }

    return false;
}

void OceanFloaterLandParts::endFloatUpDemo() {
    if (!MR::isEqualString("OceanFloaterTypeU", mObjectName)) {
        return;
    }

    if (mAlreadyDoneFlag < 0) {
        return;
    }

    if (mAlreadyDoneFlag >= 0)
        MR::onAlreadyDoneFlag(mAlreadyDoneFlag);

    if (MR::isValidSwitchA(this)) {
        MR::onSwitchA(this);
    }
}

void OceanFloaterLandParts::exeWait() {
}

void OceanFloaterLandParts::exeMove() {
    if (MR::isFirstStep(this)) {
        MR::startSound(this, "SE_OJ_HUGE_FLOATER_MOVE_ST");
        MR::emitEffect(this, "RiseBubble");

        if (MR::isEqualString(mObjectName, "OceanFloaterChurch")) {
            MR::emitEffect(this, "Rise");
        }

        MR::invalidateClipping(this);
        mIsMoving = false;
    }

    if (!mIsMoving) {
        MR::startLevelSound(this, "SE_OJ_LV_HUGE_FLOATER_MOVE");
    }

    mRailMover->movement();
    mTranslation.set(mRailMover->mTranslation); // TODO: Verify if this is right...

    if (!mRailMover->isWorking() && !mIsMoving) {
        MR::startSound(this, "SE_OJ_HUGE_FLOATER_MOVE_ED");
        MR::deleteEffect(this, "RiseBubble");

        if (MR::isEqualString(mObjectName, "OceanFloaterChurch")) {
            MR::deleteEffect(this, "Rise");
        }

        mIsMoving = true;

        if (!mIsRegisteredDemo) {
            MR::startSystemSE("SE_SY_READ_RIDDLE_S", -1, -1);
            setNerve(&NrvOceanFloaterLandParts::HostTypeDone::sInstance);
            return;
        }
    }

    if (mIsRegisteredDemo && MR::isDemoLastStep()) {
        MR::startSystemSE("SE_SY_READ_RIDDLE_S", -1, -1);
        endFloatUpDemo();
        setNerve(&NrvOceanFloaterLandParts::HostTypeDone::sInstance);
    }
}

void OceanFloaterLandParts::exeDone() {
}
