#include "TicTacToverwatchApp.h"

#include "Font.h"
#include <Input.h>
#include <algorithm>
#include <glm/ext.hpp>
#include <Gizmos.h>



TicTacToverwatchApp::TicTacToverwatchApp() {
	RunGameNetworkFrame();
	
}

TicTacToverwatchApp::~TicTacToverwatchApp() {

}

bool TicTacToverwatchApp::startup() {



	m_windowX = getWindowWidth();
	m_windowY = getWindowHeight();
	//set up open gl shader and camera. 
	m_2dRenderer = new aie::Renderer2D();
	aie::Gizmos::create(255U, 255U, 65535U, 65535U);

	//setup view
	static float aspectRatio = 16.0f / 9.0f;
	m_projection = glm::ortho<float>(-100, 100, -100 / aspectRatio, 100 / aspectRatio, -1.0f, 1.0f);
	m_view = glm::mat4(1.0f);
	m_projectionViewMatrix = m_projection * m_view;

	//set font size and style
	m_fontSize = 72; //this is used later for resizing text based on the viewport
	m_font = new aie::Font("./font/BigNoodleTooOblique.ttf", m_fontSize);

	//essentially the background
	m_playArea = new aie::Texture("./textures/TicTacToverwatch.png");
	m_highlight = new aie::Texture("./textures/highlight.png");
	//load the characters
	for (size_t i = 0; i < 23; i++)
	{
		std::string temp = "./textures/Player/" + std::to_string(i) + ".png";
		const char* path = temp.c_str();
		m_Tex[i] = new aie::Texture(path);
	}
	//load their logo's 
	for (size_t i = 0; i < 23; i++)
	{
		std::string temp = "./textures/Logo/" + std::to_string(i) + ".png";
		const char* path = temp.c_str();
		m_LogoTex[i] = new aie::Texture(path);
	}


	m_button_next = new Button(750.0f, 500.0f, 100.0f, 100.0f, "./textures/nextSymbol.png");
	m_button_previous = new Button(250.0f, 500.0f, 100.0f, 100.0f, "./textures/prevSymbol.png");
	m_button_ready = new Button(m_windowX - 257.0f, 20.0f, 257.0f, 80.0f, "./textures/ready.png");

	m_renderColour = glm::vec4(1);

	//fill the array with nothing
	for (auto c : p.name)
	{
		p.name[p.charcounter] = 0;
	}
	p.charcounter = 0;

	m_numberCharacterIcon = 1;

	//set up the tic tac toe game
	SetupBoard(m_windowX, m_windowY);
	SetupTiles(m_windowX, m_windowY);



	return true;
}

void TicTacToverwatchApp::shutdown() {

	delete m_font;
	delete m_2dRenderer;
}

void TicTacToverwatchApp::update(float deltaTime) {

	// input example
	aie::Input* input = aie::Input::getInstance();

	ScaleToViewport(getWindowWidth(), getWindowHeight());

	if (m_ready_message_display_timer > 0)
	{
		m_ready_message_display_timer -= deltaTime;
	}
	//win condition is going to be large but i can't think of a simpler way, maybe if i have an array of "Tile" structs




	if (p.ready)
	{
		//check if the mouse if hovered and highlight, if clicked, set the bool to true for drawing
		for (int i = 0; i < 9; i++)
		{
			if (isMouseHovered(tile[i].positionX, tile[i].positionY, tile[i].sizeX, tile[i].sizeY))
			{
				tile[i].mouse_hovered = true;
				if (input->wasMouseButtonPressed(0))
				{
					tile[i].clicked = true;

					//client->sendNetworkMessages(i);
				}
			}
			else
				tile[i].mouse_hovered = false;
		}
	}
	//if the player is not ready, run the "lobby"
	if (!p.ready)
	{
		SetupPlayer();

		/*
		if (isMouseHovered(m_button_next->m_position.x - (m_button_next->m_size.x * 0.5f),		//
			m_button_next->m_position.y - (m_button_next->m_size.y * 0.5f),	//	Make the button class handle hovered and clicked
			m_button_next->m_position.x + (m_button_next->m_size.x * 0.5f),		//
			m_button_next->m_position.y + (m_button_next->m_size.y * 0.5f)))
		{
			//highlight the icon...

			//check for click
			if (input->wasMouseButtonPressed(0))*/
		if (m_button_next->isHovered())
		{

			if (m_button_next->isClicked()) {
				m_numberCharacterIcon++;
				if (m_numberCharacterIcon > 22)
					m_numberCharacterIcon = 1;
			}
		}

		//}
	/*
		if (isMouseHovered(m_button_previous->m_position.x - (m_button_previous->m_size.x * 0.5f),
			m_button_previous->m_position.y - (m_button_previous->m_size.y * 0.5f),
			m_button_previous->m_position.x + (m_button_previous->m_size.x * 0.5f),
			m_button_previous->m_position.y + (m_button_previous->m_size.y * 0.5f)))
		{
			//highlight the icon...

			//check for click
			if (input->wasMouseButtonPressed(0))
	*/
		if (m_button_previous->isClicked())
		{
			m_numberCharacterIcon--;
			if (m_numberCharacterIcon < 1)
				m_numberCharacterIcon = 22;


			//}
		}

	}




	// exit the application
	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE))
		quit();
}

void TicTacToverwatchApp::draw() {

	// wipe the screen to the background colour
	clearScreen();
	setBackgroundColour(0.25f, 0.25f, 0.25f, 1.0f);
	// begin drawing sprites
	m_2dRenderer->begin();

	// draw your stuff here!

	//draw the lobby if the player is not ready
	if (!p.ready)
	{
		m_button_next->setPosition((m_windowX * 0.5f) + 250 - (m_button_next->m_size.x * 0.5f), (m_windowY * 0.5f) - (m_button_next->m_size.y * 0.5f));
		m_button_previous->setPosition((m_windowX * 0.5f) - 250 - (m_button_previous->m_size.x * 0.5f), (m_windowY * 0.5f) - (m_button_previous->m_size.y * 0.5f));
		m_button_ready->setPosition(m_windowX - 257.0f, 20.0f);


		m_2dRenderer->drawSprite(m_button_next->m_tex, m_button_next->m_position.x, m_button_next->m_position.y, m_button_next->m_size.x, m_button_next->m_size.y, 0, 0, 0.0f, 0.0f);
		m_2dRenderer->drawSprite(m_button_previous->m_tex, m_button_previous->m_position.x, m_button_previous->m_position.y, m_button_previous->m_size.x, m_button_previous->m_size.y, 0, 0, 0.0f, 0.0f);
		m_2dRenderer->drawText(m_font, " < Please Enter Your Name! > ", (m_boardLocation_X - (m_fontSize * 3)), m_windowY - 20.0f - m_fontSize, 0);
		if (p.name != nullptr)
		{
			m_2dRenderer->drawText(m_font, p.name, (m_boardLocation_X - (m_fontSize * 3)), m_windowY - 20.0f - m_fontSize - m_fontSize, 0);
		}

		m_2dRenderer->drawSprite(m_Tex[m_numberCharacterIcon], m_windowX * 0.5f, m_windowY * 0.5f, 350.0f, 350.0f, 0.0f, 0.0f, 0.5f, 0.5f);
		//set up a button to continue
		m_2dRenderer->drawSprite(m_button_ready->m_tex, m_button_ready->m_position.x, m_button_ready->m_position.y, m_button_ready->m_size.x, m_button_ready->m_size.y, 0.0f, 0.0f, 0.0f, 0.0f);


		if (m_button_ready->isClicked())
		{
			if (p.name[0] != 0)
				p.ready = true;
			else {
				m_ready_message_display_timer = 3.0f;
			}
		}
		if (m_ready_message_display_timer > 0)
			m_2dRenderer->drawText(m_font, "Please enter a name", m_windowX * 0.5f - 200, m_windowY * 0.5f, 0.0f);


		m_2dRenderer->drawText(m_font, "Press ESC to quit", 20, 20);
	}
	//draw the game if the player is ready
	if (p.ready) {
		setBackgroundColour(0.009f, 0.009f, 0.009f, 1.0f);

		//set render colour
		m_2dRenderer->setRenderColour(m_renderColour.x, m_renderColour.y, m_renderColour.z, m_renderColour.w);

		//draw the Tic Tac Toe board - TODO make these values variable relative to aspect ratio for screen resizing
		m_2dRenderer->drawSprite(m_playArea, m_boardLocation_X, m_boardLocation_Y, m_boardSize_X, m_boardSize_Y, 0.0f, 5.0f, 0.5f, 0.5f);

		// draw more stuff here
		for (auto t : tile)
		{
			if (t.mouse_hovered)
			{
				//std::string temp = std::to_string(t.positionX) + ", " + std::to_string(t.positionY);
				//const char* text = temp.c_str();
				//m_2dRenderer->drawText(m_font, text, m_boardLocation_X * 0.5f, m_boardLocation_Y * 0.5f, 0);
				m_2dRenderer->drawSprite(m_highlight, t.positionX, t.positionY, t.sizeX - t.positionX, t.sizeY - t.positionY, 0.0f, 1.0f, 0.0f, 0.0f);

			}
		}
		for (auto t : tile)
		{
			if (t.clicked)
			{
				m_2dRenderer->drawSprite(m_LogoTex[m_numberCharacterIcon], t.positionX, t.positionY, m_tileSize, m_tileSize, 0, 1, 0.0f, 0.0f);
			}
		}

		m_2dRenderer->drawSprite(m_Tex[m_numberCharacterIcon], (m_windowX * 0.5f) - (m_tileSize * 3.0f), (m_windowY * 0.5f) - (m_tileSize * 3.0f), m_tileSize, m_tileSize, 0, 1, 0.0f, 0.0f);

		//an effort to centre text when resizing was to multiply the size of the font by a quarter of the characters in the string.
		m_2dRenderer->drawText(m_font, "Tic Tac Toverwatch", (m_boardLocation_X - (m_fontSize * 3)), (m_boardSize_Y * 2) - m_fontSize, 0);

		//draw chat that scrolls up the screen TODO: make a queue to display strings, run 3 threads, display loop, inbound loop and write loop.

	}

	// done drawing sprites
	m_2dRenderer->end();

	aie::Gizmos::draw2D(m_projectionViewMatrix);


}

void TicTacToverwatchApp::ScaleToViewport(unsigned int windowX, unsigned int windowY)
{

	if (m_windowX != windowX || m_windowY != windowY)
	{
		// idk why math.h or <algorithm> would not work for me, I know they both have min functions
		float minWindow = (float)(((windowX) < (windowY)) ? (windowX) : (windowY));

		//resize the game board
		m_boardLocation_X = (float)windowX * 0.5f;
		m_boardLocation_Y = (float)windowY * 0.5f;
		m_boardSize_X = minWindow * 0.5f;
		m_boardSize_Y = minWindow * 0.5f;


		//resize the gametiles to match the board...wish i could use a "for (auto t : m_tileLocations)" here...
		m_tileSize = (minWindow * m_tileRatio);
		float halfTile = m_tileSize * 0.5f;

		//TODO get the centre tile and store temperarily, use it rather than calculating the center dor every square.

		topLeft.positionX = (float)((windowX*0.5f) - halfTile) - m_tileSize;	topLeft.positionY = (float)((windowY*0.5f) - halfTile) + m_tileSize;	topLeft.sizeX = (float)((windowX*0.5f) + halfTile) - m_tileSize;	topLeft.sizeY = (float)((windowY*0.5f) + halfTile) + m_tileSize;
		topcentre.positionX = (float)(windowX*0.5f) - halfTile;					topcentre.positionY = (float)((windowY*0.5f) - halfTile) + m_tileSize;	topcentre.sizeX = (float)(windowX*0.5f) + halfTile;					topcentre.sizeY = (float)((windowY*0.5f) + halfTile) + m_tileSize;
		topRight.positionX = (float)((windowX*0.5f) - halfTile) + m_tileSize;	topRight.positionY = (float)((windowY*0.5f) - halfTile) + m_tileSize;	topRight.sizeX = (float)((windowX*0.5f) + halfTile) + m_tileSize;	topRight.sizeY = (float)((windowY*0.5f) + halfTile) + m_tileSize;

		midLeft.positionX = (float)((windowX*0.5f) - halfTile) - m_tileSize;	midLeft.positionY = (float)(windowY*0.5f) - halfTile;					midLeft.sizeX = (float)((windowX*0.5f) + halfTile) - m_tileSize;	midLeft.sizeY = (float)(windowY*0.5f) + halfTile;
		midCentre.positionX = (float)(windowX*0.5f) - halfTile;					midCentre.positionY = (float)(windowY*0.5f) - halfTile;					midCentre.sizeX = (float)(windowX*0.5f) + halfTile;					midCentre.sizeY = (float)(windowY*0.5f) + halfTile;
		midRight.positionX = (float)((windowX*0.5f) - halfTile) + m_tileSize;	midRight.positionY = (float)(windowY*0.5f) - halfTile;					midRight.sizeX = (float)((windowX*0.5f) + halfTile) + m_tileSize;	midRight.sizeY = (float)(windowY*0.5f) + halfTile;

		botLeft.positionX = (float)((windowX*0.5f) - halfTile) - m_tileSize;	botLeft.positionY = (float)((windowY*0.5f) - halfTile) - m_tileSize;	botLeft.sizeX = (float)((windowX*0.5f) + halfTile) - m_tileSize;	botLeft.sizeY = (float)((windowY*0.5f) + halfTile) - m_tileSize;
		botCentre.positionX = (float)(windowX*0.5f) - halfTile;					botCentre.positionY = (float)((windowY*0.5f) - halfTile) - m_tileSize;	botCentre.sizeX = (float)(windowX*0.5f) + halfTile;					botCentre.sizeY = (float)((windowY*0.5f) + halfTile) - m_tileSize;
		botRight.positionX = (float)((windowX*0.5f) - halfTile) + m_tileSize;	botRight.positionY = (float)((windowY*0.5f) - halfTile) - m_tileSize;	botRight.sizeX = (float)((windowX*0.5f) + halfTile) + m_tileSize;	botRight.sizeY = (float)((windowY*0.5f) + halfTile) - m_tileSize;

		tile[0] = topLeft;
		tile[1] = topcentre;
		tile[2] = topRight;
		tile[3] = midLeft;
		tile[4] = midCentre;
		tile[5] = midRight;
		tile[6] = botLeft;
		tile[7] = botCentre;
		tile[8] = botRight;

		m_windowX = windowX, m_windowY = windowY;
	}

}

bool TicTacToverwatchApp::isMouseHovered(float minX, float minY, float maxX, float maxY)
{
	//first bit of code that i have written that makes any sense.
	aie::Input* input = aie::Input::getInstance();

	if (input->getMouseX() > minX &&
		input->getMouseY() > minY &&
		input->getMouseX() < maxX &&
		input->getMouseY() < maxY)
	{
		return true;
	}
	else
		return false;
}

bool TicTacToverwatchApp::isMouseHovered(glm::vec4 v)
{
	//good boy Brendan. 
	aie::Input* input = aie::Input::getInstance();

	if (input->getMouseX() > v.x &&
		input->getMouseY() > v.y &&
		input->getMouseX() < v.z &&
		input->getMouseY() < v.w)
	{
		return true;
	}
	else
		return false;

}

void TicTacToverwatchApp::SetupTiles(unsigned int windowX, unsigned int windowY)
{
	float minWindow = (float)(((windowX) < (windowY)) ? (windowX) : (windowY));

	m_tileSize = (minWindow * m_tileRatio);
	float halfTile = m_tileSize * 0.5f;

	topLeft.positionX = (float)((windowX*0.5f) - halfTile) - m_tileSize;	topLeft.positionY = (float)((windowY*0.5f) - halfTile) + m_tileSize;	topLeft.sizeX = (float)((windowX*0.5f) + halfTile) - m_tileSize;	topLeft.sizeY = (float)((windowY*0.5f) + halfTile) + m_tileSize;
	topcentre.positionX = (float)(windowX*0.5f) - halfTile;					topcentre.positionY = (float)((windowY*0.5f) - halfTile) + m_tileSize;	topcentre.sizeX = (float)(windowX*0.5f) + halfTile;					topcentre.sizeY = (float)((windowY*0.5f) + halfTile) + m_tileSize;
	topRight.positionX = (float)((windowX*0.5f) - halfTile) + m_tileSize;	topRight.positionY = (float)((windowY*0.5f) - halfTile) + m_tileSize;	topRight.sizeX = (float)((windowX*0.5f) + halfTile) + m_tileSize;	topRight.sizeY = (float)((windowY*0.5f) + halfTile) + m_tileSize;

	midLeft.positionX = (float)((windowX*0.5f) - halfTile) - m_tileSize;	midLeft.positionY = (float)(windowY*0.5f) - halfTile;					midLeft.sizeX = (float)((windowX*0.5f) + halfTile) - m_tileSize;	midLeft.sizeY = (float)(windowY*0.5f) + halfTile;
	midCentre.positionX = (float)(windowX*0.5f) - halfTile;					midCentre.positionY = (float)(windowY*0.5f) - halfTile;					midCentre.sizeX = (float)(windowX*0.5f) + halfTile;					midCentre.sizeY = (float)(windowY*0.5f) + halfTile;
	midRight.positionX = (float)((windowX*0.5f) - halfTile) + m_tileSize;	midRight.positionY = (float)(windowY*0.5f) - halfTile;					midRight.sizeX = (float)((windowX*0.5f) + halfTile) + m_tileSize;	midRight.sizeY = (float)(windowY*0.5f) + halfTile;

	botLeft.positionX = (float)((windowX*0.5f) - halfTile) - m_tileSize;	botLeft.positionY = (float)((windowY*0.5f) - halfTile) - m_tileSize;	botLeft.sizeX = (float)((windowX*0.5f) + halfTile) - m_tileSize;	botLeft.sizeY = (float)((windowY*0.5f) + halfTile) - m_tileSize;
	botCentre.positionX = (float)(windowX*0.5f) - halfTile;					botCentre.positionY = (float)((windowY*0.5f) - halfTile) - m_tileSize;	botCentre.sizeX = (float)(windowX*0.5f) + halfTile;					botCentre.sizeY = (float)((windowY*0.5f) + halfTile) - m_tileSize;
	botRight.positionX = (float)((windowX*0.5f) - halfTile) + m_tileSize;	botRight.positionY = (float)((windowY*0.5f) - halfTile) - m_tileSize;	botRight.sizeX = (float)((windowX*0.5f) + halfTile) + m_tileSize;	botRight.sizeY = (float)((windowY*0.5f) + halfTile) - m_tileSize;

	tile[0] = topLeft;
	tile[1] = topcentre;
	tile[2] = topRight;
	tile[3] = midLeft;
	tile[4] = midCentre;
	tile[5] = midRight;
	tile[6] = botLeft;
	tile[7] = botCentre;
	tile[8] = botRight;
}

void TicTacToverwatchApp::SetupBoard(unsigned int windowX, unsigned int windowY)
{
	float minWindow = (float)(((windowX) < (windowY)) ? (windowX) : (windowY));
	//this is the same is the code called in the update and may be redundant now.
	m_boardLocation_X = (float)windowX * 0.5f;
	m_boardLocation_Y = (float)windowY * 0.5f;
	m_boardSize_X = minWindow * 0.5f;
	m_boardSize_Y = minWindow * 0.5f;

}

void TicTacToverwatchApp::SetupPlayer()
{

	//I am sure there are a lot better ways to do this. this seems messy. TODO if i have time: clean this up to use a different way of swithching between lobby and game.
	aie::Input* input = aie::Input::getInstance();
	if (!p.ready)
	{
		//set up lobby
		for (auto c : input->getPressedCharacters())
		{
			p.name[p.charcounter] = c;
			//std::cout << m_name << std::endl;
			p.charcounter++;
			p.name[p.charcounter] = 0;
		}

		if (input->wasKeyPressed(aie::INPUT_KEY_BACKSPACE))
		{
			char temp;
			for (unsigned int i = 0; i < p.charcounter; i++)
			{
				temp = p.name[i];
				p.name[i] = temp;
			}
			//this is pointless, there will always be 1 player per client.
			p.charcounter--;
			p.name[p.charcounter] = 0;
			if (p.charcounter < 0 || p.charcounter > 256)
				p.charcounter = 0;

		}
	}

}