#include <Arduino.h>
#include <IPidAutotuner.h>
#include <PidAutotuner.h>
#include <PidController.h>

namespace {
constexpr float kTarget = 100.0f;
constexpr float kAmbient = 20.0f;
constexpr float kMaxTemperatureRise = 160.0f;
constexpr float kPlantTimeConstant = 6.0f;
constexpr float kActuatorTimeConstant = 1.0f;
constexpr float kSimulationDt = 0.05f;
constexpr unsigned long kLoopDelayMs = 10;
constexpr float kOutputCenter = 128.0f;
constexpr float kRelayStep = 45.0f;
constexpr float kNoiseBand = 0.5f;
constexpr unsigned kIgnoredHalfCycles = 2;
constexpr unsigned kAveragedHalfCycles = 8;
constexpr float kAutotuneTimeout = 120.0f;
constexpr float kSamplePeriod = 1.0f;
constexpr float kDebugPeriod = 5.0f;

PidController pid;
PidAutotuner tuner(pid);

float temperature = kAmbient;
float actuatorState = 0.0f;
float output = kOutputCenter;
float simulationTime = 0.0f;
float nextPrintTime = 0.0f;
float nextDebugTime = kDebugPeriod;
unsigned relaySwitchCount = 0;

enum class Phase {
  Autotune,
  Control,
  Failed
};

Phase phase = Phase::Autotune;

void updatePlant(float command, float dt) {
  const float normalizedCommand = command / 255.0f;
  actuatorState +=
      (normalizedCommand - actuatorState) * dt / kActuatorTimeConstant;

  const float equilibrium =
      kAmbient + kMaxTemperatureRise * actuatorState;
  temperature +=
      (equilibrium - temperature) * dt / kPlantTimeConstant;
}

const char* phaseName() {
  switch (phase) {
    case Phase::Autotune:
      return "AUTOTUNE";
    case Phase::Control:
      return "PID";
    case Phase::Failed:
      return "FAILED";
  }
  return "UNKNOWN";
}

void printSample() {
  const float error = kTarget - temperature;
  Serial.printf("%.2f,%s,%.2f,%.2f,%.2f,%.2f,%.4f,%.5f,%.5f,%.5f\r\n",
                simulationTime, phaseName(), kTarget, temperature, error,
                output, actuatorState, pid.getKp(), pid.getKi(), pid.getKd());
}

void printDebugStatus() {
  if (phase == Phase::Autotune) {
    Serial.printf("# AUTOTUNE status: t=%.2fs measurement=%.3f error=%.3f "
                  "relayOutput=%.2f switches=%u\r\n",
                  simulationTime, temperature, kTarget - temperature, output,
                  relaySwitchCount);
  } else if (phase == Phase::Control) {
    Serial.printf("# PID status: t=%.2fs measurement=%.3f error=%.3f "
                  "output=%.2f integral=%.5f\r\n",
                  simulationTime, temperature, kTarget - temperature, output,
                  pid.getIntegral());
  } else {
    Serial.printf("# FAILED status: t=%.2fs measurement=%.3f output=%.2f\r\n",
                  simulationTime, temperature, output);
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);  // Give the Wokwi terminal time to attach.
  Serial.println();
  Serial.println("# PIDReg Wokwi verification started");
  Serial.printf("# Plant: ambient=%.1f maxRise=%.1f plantTau=%.1fs "
                "actuatorTau=%.1fs modelDt=%.3fs\r\n",
                kAmbient, kMaxTemperatureRise, kPlantTimeConstant,
                kActuatorTimeConstant, kSimulationDt);
  Serial.printf("# Autotune: target=%.1f center=%.1f step=%.1f "
                "noiseBand=%.2f ignored=%u averaged=%u timeout=%.1fs "
                "rule=NoOvershoot\r\n",
                kTarget, kOutputCenter, kRelayStep, kNoiseBand,
                kIgnoredHalfCycles, kAveragedHalfCycles, kAutotuneTimeout);
  Serial.println("# CSV data follows");
  Serial.println(
      "time,phase,setpoint,measurement,error,output,actuator,kp,ki,kd");

  pid.setOutputLimits(0.0f, 255.0f);

  tuner.setTarget(kTarget);
  tuner.setOutputStep(kRelayStep);
  tuner.setNoiseBand(kNoiseBand);
  tuner.setIgnoreCycles(kIgnoredHalfCycles);
  tuner.setSettleCycles(kAveragedHalfCycles);
  tuner.setTimeout(kAutotuneTimeout);
  tuner.setOutputLimits(0.0f, 255.0f);
  tuner.setControlRule(IPidAutotuner::Rule::NoOvershoot);
  tuner.start(temperature, kOutputCenter);

  output = tuner.getLastOutput();
  Serial.printf("# Relay initial state: HIGH output=%.2f measurement=%.2f\r\n",
                output, temperature);
}

void loop() {
  updatePlant(output, kSimulationDt);
  simulationTime += kSimulationDt;

  if (phase == Phase::Autotune) {
    const float previousOutput = output;
    output = tuner.update(temperature, kSimulationDt);

    if (tuner.isRunning() && fabsf(output - previousOutput) > 0.01f) {
      relaySwitchCount++;
      Serial.printf("# Relay switch %u: state=%s t=%.2fs "
                    "measurement=%.3f error=%.3f output=%.2f\r\n",
                    relaySwitchCount,
                    output > kOutputCenter ? "HIGH" : "LOW", simulationTime,
                    temperature, kTarget - temperature, output);
    }

    if (tuner.isFinished()) {
      tuner.applyTunings();
      phase = Phase::Control;
      output = pid.compute(kTarget, temperature, kSimulationDt);

      Serial.printf("# AUTOTUNE -> PID at t=%.2fs after %u relay switches\r\n",
                    simulationTime, relaySwitchCount);
      Serial.printf("# Autotune result: Ku=%.5f Pu=%.5f "
                    "Kp=%.5f Ki=%.5f Kd=%.5f\r\n",
                    tuner.getKu(), tuner.getPu(), pid.getKp(), pid.getKi(),
                    pid.getKd());
      Serial.printf("# First PID output: measurement=%.3f error=%.3f "
                    "output=%.2f\r\n",
                    temperature, kTarget - temperature, output);
    } else if (tuner.isFailed()) {
      phase = Phase::Failed;
      output = 0.0f;
      Serial.printf("# AUTOTUNE -> FAILED at t=%.2fs after %u switches: "
                    "timeout or invalid oscillation\r\n",
                    simulationTime, relaySwitchCount);
    }
  } else if (phase == Phase::Control) {
    output = pid.compute(kTarget, temperature, kSimulationDt);
  }

  if (simulationTime >= nextPrintTime) {
    printSample();
    nextPrintTime += kSamplePeriod;
  }

  if (simulationTime >= nextDebugTime) {
    printDebugStatus();
    nextDebugTime += kDebugPeriod;
  }

  delay(kLoopDelayMs);
}
