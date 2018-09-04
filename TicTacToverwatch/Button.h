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
	void setPosition(float x, float y);


	glm::vec2 m_position;
	glm::vec2 m_size;

	aie::Texture* m_tex;

};


#endif // !BUTTON
