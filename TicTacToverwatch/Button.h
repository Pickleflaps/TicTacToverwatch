#ifndef BUTTON
#define BUTTON
#include <Texture.h>
#include <glm/common.hpp>
#include <Input.h>


class Button
{
public:
	
	Button() = delete;
	Button(float x, float y, float width, float height, const char* textureFilePath);
	Button(glm::vec2 position, glm::vec2 size, const char* textureFilePath); // position = (x,y) size = (width, height)
	Button(glm::vec4 positionAndSize, const char* textureFilePath); // vec4(xPosition, yPosition, Width, Height)
	Button(float x, float y, glm::vec2 size, const char* textureFilePath);

	virtual ~Button();

	bool isClicked();
	bool isHovered();


private:

	glm::vec2 m_position;
	glm::vec2 m_size;

	aie::Texture* m_tex;

};

inline Button::Button(float x, float y, float width, float height, const char * textureFilePath) 
	: m_position{ x, y }
	, m_size{ width, height }
	, m_tex{ new aie::Texture(textureFilePath) }
{
}

inline Button::Button(glm::vec2 position, glm::vec2 size, const char * textureFilePath) 
	: m_position{ position }
	, m_size{ size }
	, m_tex{ new aie::Texture(textureFilePath) }
{
}

inline Button::Button(glm::vec4 positionAndSize, const char * textureFilePath)
	: m_position{ positionAndSize.x, positionAndSize.y }
	, m_size{ positionAndSize.z, positionAndSize.w }
	, m_tex{ new aie::Texture(textureFilePath) }
{
}

inline Button::Button(float x, float y, glm::vec2 size, const char * textureFilePath)
	: m_position{ x, y }
	, m_size{ size }
	, m_tex{ new aie::Texture(textureFilePath) }
{
}

Button::~Button()
{
	delete m_tex;
	
}

inline bool Button::isClicked()
{
	aie::Input* input = aie::Input::getInstance();
	if(input->wasMouseButtonReleased(aie::INPUT_MOUSE_BUTTON_LEFT) && isHovered())
	{
		return true;
	}
	return false;
}

inline bool Button::isHovered()
{
	aie::Input* input = aie::Input::getInstance();
	int x, y;
	input->getMouseXY(&x, &y);
	if (x >= m_position.x && x <= m_position.x + m_size.x &&
		y >= m_position.y && y <= m_position.y + m_size.y)
	{
		return true;
	}
	return false;
}


#endif // !BUTTON
