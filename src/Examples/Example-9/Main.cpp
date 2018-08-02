﻿#include <Magma/Framework/Input/GLWindow.hpp>
#include <Magma/Framework/Graphics/OGL410RenderDevice.hpp>

#include <Magma/Framework/Input/D3DWindow.hpp>
#include <Magma/Framework/Graphics/D3D11RenderDevice.hpp>

#include <Magma/Framework/Files/STDFileSystem.hpp>
#include <Magma/Framework/String/Conversion.hpp>
#include <iostream>

#include <Magma/GUI/Renderer.hpp>
#include <Magma/GUI/Elements/Box.hpp>

#define USE_GL

using namespace Magma;

struct Scene
{
	Framework::Input::Window* window;
	Framework::Graphics::RenderDevice* device;
	Framework::Files::FileSystem* fileSystem;
	bool running;

	Framework::Graphics::RasterState* rasterState;
	Framework::Graphics::DepthStencilState* depthStencilState;
	Framework::Graphics::BlendState* blendState;

	GUI::Root* guiRoot;
	GUI::Renderer* guiRenderer;
};

void LoadScene(Scene& scene)
{
	// Create filesystem
	{
		scene.fileSystem = new Framework::Files::STDFileSystem("../../../../../resources/");
	}

	// Create window
	{
#ifdef USE_GL
		scene.window = new Framework::Input::GLWindow(800, 600, "Example-9", Framework::Input::Window::Mode::Windowed);
#else
		scene.window = new Framework::Input::D3DWindow(800, 600, "Example-9", Framework::Input::Window::Mode::Windowed);
#endif
		scene.running = true;
		scene.window->OnClose.AddListener([&scene]() { scene.running = false; });
	}

	// Create context
	{
		Framework::Graphics::RenderDeviceSettings settings;
#ifdef USE_GL
		scene.device = new Framework::Graphics::OGL410RenderDevice();
#else
		scene.device = new Framework::Graphics::D3D11RenderDevice();
#endif
		scene.device->Init(scene.window, settings);
	}

	// Create raster state
	{
		Framework::Graphics::RasterStateDesc desc;

		desc.rasterMode = Framework::Graphics::RasterMode::Fill;
		desc.cullEnabled = false;

		scene.rasterState = scene.device->CreateRasterState(desc);
	}

	// Create depth stencil state
	{
		Framework::Graphics::DepthStencilStateDesc desc;
		scene.depthStencilState = scene.device->CreateDepthStencilState(desc);
	}

	// Create blend state
	{
		Framework::Graphics::BlendStateDesc desc;
		desc.blendEnabled = true;
		desc.sourceFactor =	Framework::Graphics::BlendFactor::SourceAlpha;
		desc.destinationFactor = Framework::Graphics::BlendFactor::InverseSourceAlpha;
		scene.blendState = scene.device->CreateBlendState(desc);
	}

	// Create GUI Root
	{
		scene.guiRoot = new GUI::Root();
	}

	// Create GUI Renderer
	{
		scene.guiRenderer = new GUI::Renderer(scene.device);
	}

	// Create GUI box element
	{
		auto element = scene.guiRoot->CreateElement<GUI::Elements::Box>(nullptr, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

		GUI::BoundingBox bb;

		bb.left.absolute = 0.0f;
		bb.left.relative = 0.5f;

		bb.right.absolute = 50.0f;
		bb.right.relative = 0.5f;

		bb.bottom.absolute = 0.0f;
		bb.bottom.relative = 0.5f;

		bb.top.absolute = 50.0f;
		bb.top.relative = 0.5f;

		element->SetBox(bb);
	}
}

void CleanScene(Scene& scene)
{
	// Delete GUI stuff
	delete scene.guiRenderer;
	delete scene.guiRoot;

	// Clean render device objects
	scene.device->DestroyBlendState(scene.blendState);
	scene.device->DestroyDepthStencilState(scene.depthStencilState);
	scene.device->DestroyRasterState(scene.rasterState);

	// Destroy context, window and filesytem
	delete scene.device;
	delete scene.window;
	delete scene.fileSystem;
}

void Main(int argc, char** argv) try
{
	Scene scene;

	LoadScene(scene);

	// Main loop
	while (scene.running)
	{
		// Poll events
		scene.window->PollEvents();

		// Clear screen
		scene.device->Clear(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		// Set raster and depth stencil states
		scene.device->SetRasterState(scene.rasterState);
		scene.device->SetDepthStencilState(scene.depthStencilState);
		scene.device->SetBlendState(scene.blendState);

		// Draw GUI
		scene.guiRenderer->Render(scene.guiRoot);

		// Swap screen back and front buffers
		scene.device->SwapBuffers();
	}

	CleanScene(scene);
}
catch (Framework::Graphics::ShaderError& e)
{
	std::cout << "Shader error caught:" << std::endl;
	std::cout << e.what() << std::endl;
	getchar();
}
catch (Framework::Graphics::RenderDeviceError& e)
{
	std::cout << "Render device error caught:" << std::endl;
	std::cout << e.what() << std::endl;
	getchar();
}
catch (Framework::Graphics::TextError& e)
{
	std::cout << "Text error caught:" << std::endl;
	std::cout << e.what() << std::endl;
	getchar();
}