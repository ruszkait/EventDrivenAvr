#define GLUE(a, b)     a##b
#define GLUE3(a, b, c)     a##b##c
#define PORT(x)        GLUE(PORT, x)
#define PIN(x)         GLUE(PIN, x)
#define PINBIT(portName, bitPos)         GLUE3(PIN, portName, bitPos)
#define DDR(x)         GLUE(DDR, x)
