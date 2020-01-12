#include "LightClass.h"

LightClass::LightClass()
{
}

LightClass::LightClass(const LightClass&)
{
}

LightClass::~LightClass()
{
}

void LightClass::SetDiffuseColor(float red, float green, float blue, float alpha)
{
	m_diffuseColor.x = red;
	m_diffuseColor.y = green;
	m_diffuseColor.z = blue;
	m_diffuseColor.w = alpha;
}

void LightClass::SetDirection(float x, float y, float z)
{
	m_direction.x = x;
	m_direction.y = y;
	m_direction.z = z;
}

XMFLOAT4 LightClass::GetDiffuseColor()
{
	return m_diffuseColor;
}

XMFLOAT3 LightClass::GetDirection()
{
	return m_direction;
}
