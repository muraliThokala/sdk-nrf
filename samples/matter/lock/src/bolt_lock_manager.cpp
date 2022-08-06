/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bolt_lock_manager.h"

#include "app_event.h"
#include "app_task.h"

using namespace chip;

BoltLockManager BoltLockManager::sLock;

void BoltLockManager::Init(StateChangeCallback callback)
{
	mStateChangeCallback = callback;

	k_timer_init(&mActuatorTimer, &BoltLockManager::ActuatorTimerEventHandler, nullptr);
	k_timer_user_data_set(&mActuatorTimer, this);
}

bool BoltLockManager::GetUser(uint16_t userIndex, EmberAfPluginDoorLockUserInfo &user) const
{
	/* userIndex is guaranteed by the caller to be between 1 and CONFIG_LOCK_NUM_USERS */
	user = mUsers[userIndex - 1];

	ChipLogProgress(Zcl, "Getting lock user %u: %s", static_cast<unsigned>(userIndex),
			user.userStatus == DlUserStatus::kAvailable ? "available" : "occupied");

	return true;
}

bool BoltLockManager::SetUser(uint16_t userIndex, FabricIndex creator, FabricIndex modifier, const CharSpan &userName,
			      uint32_t uniqueId, DlUserStatus userStatus, DlUserType userType,
			      DlCredentialRule credentialRule, const DlCredential *credentials, size_t totalCredentials)
{
	/* userIndex is guaranteed by the caller to be between 1 and CONFIG_LOCK_NUM_USERS */
	UserData &userData = mUserData[userIndex - 1];
	auto &user = mUsers[userIndex - 1];

	VerifyOrReturnError(userName.size() <= DOOR_LOCK_MAX_USER_NAME_SIZE, false);
	VerifyOrReturnError(totalCredentials <= CONFIG_LOCK_NUM_CREDENTIALS_PER_USER, false);

	Platform::CopyString(userData.mName, userName);
	memcpy(userData.mCredentials, credentials, totalCredentials * sizeof(DlCredential));

	user.userName = CharSpan(userData.mName, userName.size());
	user.credentials = Span<const DlCredential>(userData.mCredentials, totalCredentials);
	user.userUniqueId = uniqueId;
	user.userStatus = userStatus;
	user.userType = userType;
	user.credentialRule = credentialRule;
	user.creationSource = DlAssetSource::kMatterIM;
	user.createdBy = creator;
	user.modificationSource = DlAssetSource::kMatterIM;
	user.lastModifiedBy = modifier;

	ChipLogProgress(Zcl, "Setting lock user %u: %s", static_cast<unsigned>(userIndex),
			userStatus == DlUserStatus::kAvailable ? "available" : "occupied");

	return true;
}

bool BoltLockManager::GetCredential(uint16_t credentialIndex, DlCredentialType credentialType,
				    EmberAfPluginDoorLockCredentialInfo &credential) const
{
	VerifyOrReturnError(credentialIndex > 0 && credentialIndex <= CONFIG_LOCK_NUM_CREDENTIALS, false);

	credential = mCredentials[credentialIndex - 1];

	ChipLogProgress(Zcl, "Getting lock credential %u: %s", static_cast<unsigned>(credentialIndex),
			credential.status == DlCredentialStatus::kAvailable ? "available" : "occupied");

	return true;
}

bool BoltLockManager::SetCredential(uint16_t credentialIndex, FabricIndex creator, FabricIndex modifier,
				    DlCredentialStatus credentialStatus, DlCredentialType credentialType,
				    const ByteSpan &secret)
{
	VerifyOrReturnError(credentialIndex > 0 && credentialIndex <= CONFIG_LOCK_NUM_CREDENTIALS, false);
	VerifyOrReturnError(secret.size() <= kMaxCredentialLength, false);

	CredentialData &credentialData = mCredentialData[credentialIndex - 1];
	auto &credential = mCredentials[credentialIndex - 1];

	if (!secret.empty()) {
		memcpy(credentialData.mSecret.Alloc(secret.size()).Get(), secret.data(), secret.size());
	}

	credential.status = credentialStatus;
	credential.credentialType = credentialType;
	credential.credentialData = ByteSpan(credentialData.mSecret.Get(), secret.size());
	credential.creationSource = DlAssetSource::kMatterIM;
	credential.createdBy = creator;
	credential.modificationSource = DlAssetSource::kMatterIM;
	credential.lastModifiedBy = modifier;

	ChipLogProgress(Zcl, "Setting lock credential %u: %s", static_cast<unsigned>(credentialIndex),
			credential.status == DlCredentialStatus::kAvailable ? "available" : "occupied");

	return true;
}

bool BoltLockManager::ValidatePIN(const Optional<ByteSpan> &pinCode, DlOperationError &err) const
{
	/* Optionality of the PIN code is validated by the caller, so assume it is OK not to provide the PIN code. */
	if (!pinCode.HasValue()) {
		return true;
	}

	/* Check the PIN code */
	for (const auto &credential : mCredentials) {
		if (credential.status == DlCredentialStatus::kAvailable ||
		    credential.credentialType != DlCredentialType::kPin) {
			continue;
		}

		if (credential.credentialData.data_equal(pinCode.Value())) {
			ChipLogDetail(Zcl, "Valid lock PIN code provided");
			return true;
		}
	}

	ChipLogDetail(Zcl, "Invalid lock PIN code provided");
	err = DlOperationError::kInvalidCredential;

	return false;
}

void BoltLockManager::Lock(OperationSource source)
{
	VerifyOrReturn(mState != State::kLockingCompleted);
	SetState(State::kLockingInitiated, source);

	mActuatorOperationSource = source;
	k_timer_start(&mActuatorTimer, K_MSEC(kActuatorMovementTimeMs), K_NO_WAIT);
}

void BoltLockManager::Unlock(OperationSource source)
{
	VerifyOrReturn(mState != State::kUnlockingCompleted);
	SetState(State::kUnlockingInitiated, source);

	mActuatorOperationSource = source;
	k_timer_start(&mActuatorTimer, K_MSEC(kActuatorMovementTimeMs), K_NO_WAIT);
}

void BoltLockManager::ActuatorTimerEventHandler(k_timer *timer)
{
	/*
	 * The timer event handler is called in the context of the system clock ISR.
	 * Post an event to the application task queue to process the event in the
	 * context of the application thread.
	 */

	GetAppTask().PostEvent(AppEvent(AppEvent::CompleteLockAction, BoltLockManager::OperationSource{}));
}

void BoltLockManager::CompleteLockAction()
{
	switch (mState) {
	case State::kLockingInitiated:
		SetState(State::kLockingCompleted, mActuatorOperationSource);
		break;
	case State::kUnlockingInitiated:
		SetState(State::kUnlockingCompleted, mActuatorOperationSource);
		break;
	default:
		break;
	}
}

void BoltLockManager::SetState(State state, OperationSource source)
{
	mState = state;

	if (mStateChangeCallback != nullptr) {
		mStateChangeCallback(state, source);
	}
}
