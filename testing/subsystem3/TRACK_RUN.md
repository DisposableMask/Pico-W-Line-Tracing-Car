# Slow IR track run

Status: **Pending hardware test and motor integration** ([#11](https://github.com/DisposableMask/Pico-W-Line-Tracing-Car/issues/11)).

1. Disable the motors. With the sensors at driving height, hold tape near each switching point. Record whether the output flickers and adjust the comparator if needed.
2. Once the motor task consumes IR steering decisions, check wheel direction with the wheels raised. Check that an invalid IR snapshot commands stop.
3. At low speed, drive a straight and then one gentle curve in each direction. On both curves the car should correct toward the line without rapid left/right switching.

| Check | Result |
|---|---|
| Comparator stability | Pending |
| Wheels-up direction and invalid snapshot | Pending |
| Straight | Pending |
| Left curve | Pending |
| Right curve | Pending |
