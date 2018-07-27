#pragma once

#include "Application.h"
#include "Renderer2D.h"
// #include "Button.h"


class TicTacToverwatchApp : public aie::Application {
public:

	TicTacToverwatchApp();
	virtual ~TicTacToverwatchApp();

	virtual bool startup();
	virtual void shutdown();

	virtual void update(float deltaTime);
	virtual void draw();

protected:

	aie::Renderer2D*	m_2dRenderer;
	aie::Font*			m_font;


	aie::Texture*		m_playArea;
	aie::Texture*		m_X;
	aie::Texture*		m_highlight;
	aie::Texture*		m_otherplayer;
	aie::Texture*		m_Tex[23];
	aie::Texture*		m_LogoTex[23];

	//Button*			m_button_next;
	//Button*			m_button_previous;



};