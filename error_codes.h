#pragma once

#include "X_macro_helpers.h"

#include <stdbool.h>

// Fault vs. Error included in names for clarity of behavior at call site
// and when reading messages, logs.

#define BE_FOREACH_BLDC_ERROR(X)             \
    X(PumpMovedTwoStepsForwardError        ) \
    X(PumpMovedThreeStepsForwardError      ) \
    X(PumpMovedTwoStepsBackwardError       ) \
    X(PumpMovedThreeStepsBackwardError     ) \
    X(PumpMovedInReverseError              ) \
    X(PumpAlreadyRunningError              ) \
    X(PumpUpperSlewRateLimitExceededError  ) \
    X(PumpLowerSlewRateLimitExceededError  ) \
    X(PumpOverCurrentDetectedWhileIdleError) \
    X(FaultPreventsPumpRunningError        ) \
    X(CAFEFaultPreventsPumpRunningError    ) \

#define BE_FOREACH_HANDLE_ERROR(X)               \
    X(HandleUSPInvalid__msg_type__ReceivedError) \
    X(HandleUSPInvalid__payload__ReceivedError ) \
    X(HandleUSPUnhandledMessageError           ) \
    X(HandleUmbilicalOverrunError              ) \
    X(HandleUmbilicalParityError               ) \
    X(HandleUmbilicalFramingError              ) \
    X(HandleUmbilicalBreakError                ) \
    X(HandleIOPinConfigError                   ) \
    X(IncompatibleHandlePCBAError              ) \

#define BE_FOREACH_HANDLE_FAULT(X)   \
    X(HandleFirmwareChecksumFault  ) \
    X(HandleFSPCallFailedFault     ) \
    X(Handle_AMB_TEMP_OverTempFault) \
    X(Handle_MTR_TEMP_OverTempFault) \

#define BE_FOREACH_ERROR(X)                      \
    X(NoError                                  ) \
    X(NotCalibratedError                       ) \
    X(HardwareResetError                       ) \
    X(WatchDogResetError                       ) \
    X(MCU_TemperatureWarningError              ) \
    X(HandleConnectionFailedError              ) \
    X(FailedToSetHandleLowPowerModeError       ) \
    X(FailedToUnsetHandleLowPowerModeError     ) \
    X(HandleNotResponding                      ) \
    X(BasePSPInvalid__msg_type__ReceivedError  ) \
    X(BasePSPInvalid__payload__ReceivedError   ) \
    X(BasePSPInvalid__msg_type__ProcessCmdError) \
    X(BasePSPInvalidUnhandledProcessCmdError   ) \
    X(BasePSPUnhandledMessageError             ) \
    X(BaseShellSerialOverrunError              ) \
    X(BaseShellSerialParityError               ) \
    X(BaseShellSerialFramingError              ) \
    X(BaseShellSerialBreakError                ) \
    X(BaseUmbilicalOverrunError                ) \
    X(BaseUmbilicalParityError                 ) \
    X(BaseUmbilicalFramingError                ) \
    X(BaseUmbilicalBreakError                  ) \
    X(BaseUmbilicalSendCollisionError          ) \
    X(BaseUmbilicalNoResponseFromHandleError   ) \
    X(BaseFlashErasePageOutOfRangeError        ) \
    X(LogBufferOverflowError                   ) \
    X(HandleSentBaseErrorCodeError             ) \
    X(IncompatibleBasePCBAError                ) \
    X(ConfigChecksumIncorrectError             ) \
    X(ConfigNotBackwardsCompatibleError        ) \
    X(ConfigNotSetError                        ) \
    X(CAFEFirstReadAllOnesError                ) \
    X(CAFEInitFailedError                      ) \
    X(V48_MON_LowError                         ) \
    BE_FOREACH_BLDC_ERROR(X)                     \
    BE_FOREACH_HANDLE_ERROR(X) /* last! */       \

#define BE_FOREACH_BLDC_FAULT(X)              \
    X(Invalid__bldc_state__Fault            ) \
    X(PumpMotorFailedToStartFault           ) \
    X(PumpMotorFailedToStopFault            ) \
    X(PumpMotorHallsAllOffFault             ) \
    X(PumpMotorHallsAllOnFault              ) \
    X(PumpMotorStalledAtStartupFault        ) \
    X(PumpMotorStalledWhileAcceleratingFault) \
    X(PumpMotorStalledWhileDeceleratingFault) \
    X(PumpMotorStalledWhileRunningFault     ) \
    X(PumpMotorSyncLostFault                ) \
    X(PumpMotorOverCurrentFault             ) \
    X(PumpMotorOverTempFWDetectedFault      ) \
    X(PumpMotorOverTempHWDetectedFault      ) \
    X(PumpDriveOverTempFault                ) \

#define BE_FOREACH_FAULT(X)               \
    /* ARM faults */                      \
    X(DataAccessViolationMemFault       ) \
    X(DivideByZeroFault                 ) \
    X(ExceptionStackingBusFault         ) \
    X(ExceptionStackingMemFault         ) \
    X(ExceptionUnstackingBusFault       ) \
    X(ExceptionUnstackingMemFault       ) \
    X(FPLazyStatePreservationMemFault   ) \
    X(ForcedHardFault                   ) \
    X(ImpreciseDataBusFault             ) \
    X(InstructionAccessViolationMemFault) \
    X(InstructionBusFault               ) \
    X(InvalidEPSRUsageFault             ) \
    X(InvalidPCFault                    ) \
    X(NoCoprocessorFault                ) \
    X(PreciseDataBusFault               ) \
    X(UnalignedAccessFault              ) \
    X(UndefinedInstructionFault         ) \
    X(VectorTableReadHardFault          ) \
    /* PWRSTAT Faults */                  \
    X(MCU_VCORE_Fault                   ) \
    X(MCU_VCC3_3_Fault                  ) \
    X(MCU_VCCIO_Fault                   ) \
    X(MCU_VSYS_Fault                    ) \
    X(MCU_TemperatureFault              ) \
    X(MCU_DCDC_Fault                    ) \
    /* App faults */                      \
    X(Invalid__app_state__Fault         ) \
    /* Base ADC faults */                 \
    X(CAFEInitFailedFault               ) \
    X(CAFEReadBackFault                 ) \
    /* Misc faults */                     \
    X(BaseFirmwareChecksumFault         ) \
    X(BaseHandleVersionMismatchFault    ) \
    X(BaseStackOverflowFault            ) \
    X(DiskFailedtoStartFault            ) \
    X(DiskUnderSpeedFault               ) \
    X(HandleConnectionFailedFault       ) \
    X(UserRequestedFault                ) \
    X(REF_2V4_MON_LowFault              ) \
    X(V48_MON_LowFault                  ) \
    BE_FOREACH_BLDC_FAULT(X)              \
    BE_FOREACH_HANDLE_FAULT(X)            \

#define BE_FOREACH_ERROR_CODE(X) \
    BE_FOREACH_ERROR(X)          \
    BE_FOREACH_FAULT(X)          \

#define BE_FOREACH_HANDLE_ERROR_CODE(X) \
    BE_FOREACH_HANDLE_ERROR(X)          \
    BE_FOREACH_HANDLE_FAULT(X)          \

#define BE_FOREACH_BLDC_ERROR_CODE(X) \
    BE_FOREACH_BLDC_ERROR(X)          \
    BE_FOREACH_BLDC_FAULT(X)          \

typedef enum {
    BE_FOREACH_ERROR_CODE(DECLARE_NAME)
} error_code_t;

_Static_assert(sizeof(error_code_t) == 1, "Error codes must fit in a byte");

#define err_lowest_fault_error_code M_SELECT_ARG(0, M_FIRST_DECL_IN(BE_FOREACH_FAULT))

#define err_num_error_codes M_NUM_DECLS_IN(BE_FOREACH_ERROR_CODE)

#define BE_GENERATE_bf_unset_CALL(error_code) \
    be_unset(error_code);

static ALWAYS_INLINE bool err_is_fault(error_code_t error_code) {
    return error_code >= err_lowest_fault_error_code;
}

static inline bool err_is_handle_error_code(error_code_t error_code) {
    return (
        (
               error_code >= M_FIRST_DECL_IN(BE_FOREACH_HANDLE_ERROR)
            && error_code <= M_LAST_DECL_IN( BE_FOREACH_HANDLE_ERROR)
        )
        || (
               error_code >= M_FIRST_DECL_IN(BE_FOREACH_HANDLE_FAULT)
            && error_code <= M_LAST_DECL_IN( BE_FOREACH_HANDLE_FAULT)
        )
    );
}
