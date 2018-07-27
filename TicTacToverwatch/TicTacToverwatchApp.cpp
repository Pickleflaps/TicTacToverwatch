#include "TicTacToverwatchApp.h"
#include "Texture.h"
#include "Font.h"
#include "Input.h"


TicTacToverwatchApp::TicTacToverwatchApp() {

}

TicTacToverwatchApp::~TicTacToverwatchApp() {

}

bool TicTacToverwatchApp::startup() {
	//set up open gl shader and camera. 
	m_2dRenderer = new aie::Renderer2D();

	//set font size and style
	m_font = new aie::Font("./font/BigNoodleTooOblique.ttf", 72);

	//essentially the background
	m_playArea = new aie::Texture("./textures/TicTacToverwatch.png");
	m_highlight = new aie::Texture("./textures/highlight.png");

	//set up the next and previous button for the character select screen
	//glm::vec2 buttonSize(100, 100);
	//m_button_next = new Button(750, 250, buttonSize, "./textures/nextSymbol.png");
	//m_button_previous = new Button(250, 750, buttonSize, "./textures/prevSymbol.png");





	return true;
}

void TicTacToverwatchApp::shutdown() {

	delete m_font;
	delete m_2dRenderer;
}

void TicTacToverwatchApp::update(float deltaTime) {

	// input example
	aie::Input* input = aie::Input::getInstance();

	// exit the application
	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE))
		quit();
}

void TicTacToverwatchApp::draw() {

	// wipe the screen to the background colour
	clearScreen();

	// begin drawing sprites
	m_2dRenderer->begin();

	// draw your stuff here!
	
	// output some text, uses the last used colour
	m_2dRenderer->drawText(m_font, "Press ESC to quit", 0, 0);

	// done drawing sprites
	m_2dRenderer->end();
}