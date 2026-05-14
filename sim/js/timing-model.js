export class TimingModel {
  constructor() {
    this.rpm = 1200;
    this.rpmJitter = 0;
    this.hallJitterUs = 0;
    this.hallMissRate = 0;
    this.sliceTimerDriftPpm = 0;
    this.patternLagMs = 0;
    this.spiClockMhz = 20;
    this.numLeds = 144;
    this.numSlices = 360;
    this.patternFps = 30;

    this._lastHallTimeMs = 0;
    this._currentPeriodMs = 0;
    this._frameAge = 0;
    this._lastPatternGenMs = -Infinity;
    this._revolutionCount = 0;
  }

  get revolutionPeriodMs() {
    return 60000 / this.rpm;
  }

  get sliceIntervalUs() {
    return (this.revolutionPeriodMs * 1000) / this.numSlices;
  }

  spiTransferUs() {
    const bytes = 4 + this.numLeds * 4 + Math.ceil(this.numLeds / 16);
    const bits = bytes * 8;
    return bits / (this.spiClockMhz * 1e6) * 1e6;
  }

  get headroomUs() {
    return this.sliceIntervalUs - this.spiTransferUs();
  }

  patternGenTimeMs() {
    const baseMs = (this.numSlices * this.numLeds * 100e-9) * 1000;
    return baseMs + this.patternLagMs;
  }

  _gaussRandom() {
    let u = 0, v = 0;
    while (u === 0) u = Math.random();
    while (v === 0) v = Math.random();
    return Math.sqrt(-2.0 * Math.log(u)) * Math.cos(2.0 * Math.PI * v);
  }

  simulateRevolution(simTimeMs) {
    this._revolutionCount++;

    const jitterFactor = 1 + this._gaussRandom() * (this.rpmJitter / 100);
    const actualRpm = this.rpm * Math.max(0.1, jitterFactor);
    const periodMs = 60000 / actualRpm;
    this._currentPeriodMs = periodMs;

    const hallOffsetUs = this._gaussRandom() * this.hallJitterUs;
    const hallMissed = Math.random() < this.hallMissRate;

    const patternGenTime = this.patternGenTimeMs();
    const timeSinceLastGen = simTimeMs - this._lastPatternGenMs;
    const patternInterval = 1000 / this.patternFps;
    let patternReady = true;

    if (timeSinceLastGen >= patternInterval) {
      if (patternGenTime > patternInterval) {
        this._frameAge++;
        patternReady = false;
      } else {
        this._frameAge = 0;
        this._lastPatternGenMs = simTimeMs;
      }
    }

    const sliceIntervalUs = (periodMs * 1000) / this.numSlices;
    const spiUs = this.spiTransferUs();
    const slices = [];
    let cumulativeDriftUs = 0;

    for (let i = 0; i < this.numSlices; i++) {
      const nominalAngle = (i / this.numSlices) * Math.PI * 2;

      const driftPerSlice = sliceIntervalUs * (this.sliceTimerDriftPpm / 1e6);
      cumulativeDriftUs += driftPerSlice;

      const totalOffsetUs = hallOffsetUs + cumulativeDriftUs;
      const offsetFraction = totalOffsetUs / (periodMs * 1000);
      const actualAngle = nominalAngle + offsetFraction * Math.PI * 2;

      const overrun = spiUs > sliceIntervalUs;

      slices.push({
        sliceIndex: i,
        nominalAngle,
        actualAngle,
        overrun,
      });
    }

    return {
      periodMs,
      hallMissed,
      hallOffsetUs,
      patternReady,
      frameAge: this._frameAge,
      slices,
      spiTransferUs: spiUs,
      sliceIntervalUs,
      headroomUs: sliceIntervalUs - spiUs,
      actualRpm: actualRpm,
    };
  }

  reset() {
    this._lastHallTimeMs = 0;
    this._currentPeriodMs = 0;
    this._frameAge = 0;
    this._lastPatternGenMs = -Infinity;
    this._revolutionCount = 0;
  }
}
