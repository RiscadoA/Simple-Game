#pragma once

#include "Window.hpp"
#include "../OGLWindow.h"
#include "../../String/UTF8.hpp"

namespace Magma
{
	namespace Framework
	{
		namespace Input
		{
			/// <summary>
			///		WindowHandle class implementation for OpenGL (uses GLFW)
			/// </summary>
			class OGLWindow : public HWindow
			{
			public:
				/// <summary>
				///		Opens a window (OpenGL implementation).
				/// </summary>
				/// <param name="width">WindowHandle width</param>
				/// <param name="height">WindowHandle height</param>
				/// <param name="title">WindowHandle title</param>
				/// <param name="mode">WindowHandle mode</param>
				OGLWindow(mfmU32 width, mfmU32 height, const String::UTF8CodeUnit* title, HWindow::Mode mode = HWindow::Mode::Windowed);

				/// <summary>
				///		Closes a window.
				/// </summary>
				~OGLWindow();

				/// <summary>
				///		Polls events from this window.
				/// </summary>
				virtual void PollEvents() final;

				/// <summary>
				///		Waits for an event and handles it.
				/// </summary>
				virtual void WaitForEvents() final;

				/// <summary>
				///		Gets the width of the window.
				/// </summary>
				/// <returns>WindowHandle width</returns>
				inline virtual mfmU32 GetWidth() final { return m_window->getWidth(m_window); }

				/// <summary>
				///		Gets the height of the window.
				/// </summary>
				/// <returns>WindowHandle height</returns>
				inline virtual mfmU32 GetHeight() final { return m_window->getHeight(m_window); }

				/// <summary>
				///		Gets the window mode
				/// </summary>
				/// <returns>WindowHandle mode</returns>
				inline virtual HWindow::Mode GetMode() final { return (HWindow::Mode)m_window->getMode(m_window); }

				/// <summary>
				///		Returns a pointer to the underlying C window
				/// </summary>
				/// <returns>C window pinter</returns>
				inline mfiWindow* GetWindow() const { return m_window; }

			private:
				mfiWindow* m_window;
			};
		}
	}
}