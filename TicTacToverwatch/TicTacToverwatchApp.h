#pragma once

#include "Application.h"
#include "Renderer2D.h"
#include "Button.h"
#include <glm/mat4x4.hpp>
#include <../GameNetworkFrame/GameNetworkFrame.h>



struct Tile
{
	float positionX = 0;
	float positionY = 0;
	float sizeX = 0;
	float sizeY = 0;
	bool mouse_hovered = false;
	bool clicked = false;
	bool shouldUpdate = false;

	enum Status
	{
		EMPTY,
		PLAYERX,
		PLAYERY,

	};


};
struct Player
{
	unsigned int	charcounter = 0;
	char			name[256];
	int				score = 0;
	std::string		PlayerSprite = "./textures/X.png";
	bool			ready = false;
	enum auth
	{
		PLAYER1,
		PLAYER2,
		SPECTATOR,

	};

};

class TicTacToverwatchApp : public aie::Application {
public:

	TicTacToverwatchApp();
	virtual ~TicTacToverwatchApp();

	virtual bool startup();
	virtual void shutdown();

	virtual void update(float deltaTime);
	virtual void draw();

	//only call once
	void SetupTiles(unsigned int windowX, unsigned int windowY);
	void SetupBoard(unsigned int windowX, unsigned int windowY);
	void SetupPlayer();
	//call on update
	void ScaleToViewport(unsigned int windowX, unsigned int windowY);

	bool isMouseHovered(float minX, float minY, float maxX, float maxY);
	bool isMouseHovered(glm::vec4 v);



protected:

	GameNetworkFrame * GNF;

	Button *			m_button_next;
	Button*				m_button_previous;
	Button*				m_button_ready;

	aie::Renderer2D*	m_2dRenderer;
	aie::Font*			m_font;



	aie::Texture*		m_playArea;
	aie::Texture*		m_highlight;
	aie::Texture*		m_Tex[23];
	aie::Texture*		m_LogoTex[23];

private:

	unsigned int		m_numberCharacterIcon = 0;
	int					m_fontSize;
	unsigned int		m_windowX, m_windowY = 500;
	float				m_tileSize;
	float				m_boardLocation_X;
	float				m_boardLocation_Y;
	float				m_boardSize_X;
	float				m_boardSize_Y;
	glm::vec4			m_renderColour;

	float				m_ready_message_display_timer = 0;

	//gotta love magic numbers
	const float			m_tileRatio = 0.163987f;

	Tile topLeft;
	Tile topcentre;
	Tile topRight;
	Tile midLeft;
	Tile midCentre;
	Tile midRight;
	Tile botLeft;
	Tile botCentre;
	Tile botRight;

	Tile tile[9];

	Player p;

	glm::mat4			m_projection;
	glm::mat4			m_view;
	glm::mat4			m_projectionViewMatrix;


};