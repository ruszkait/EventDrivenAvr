#ifndef MESSAGES
#define MESSAGES


// ***********************************************************
// Command messages
//

struct LEDMessageParam
{
	uint8_t m_LEDId;
	bool m_SwitchOn;
};

struct KeyPressParam
{
	uint8_t m_keyCode;
};

#endif


