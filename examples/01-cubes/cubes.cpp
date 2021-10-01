/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"
#include <array>

namespace
{

struct PosColorVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_abgr;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
			.end();
	};

	static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorVertex::ms_layout;

static PosColorVertex s_cubeVertices[] =
{
	{-1.0f,  1.0f,  1.0f, 0xff000000 },
	{ 1.0f,  1.0f,  1.0f, 0xff0000ff },
	{-1.0f, -1.0f,  1.0f, 0xff00ff00 },
	{ 1.0f, -1.0f,  1.0f, 0xff00ffff },
	{-1.0f,  1.0f, -1.0f, 0xffff0000 },
	{ 1.0f,  1.0f, -1.0f, 0xffff00ff },
	{-1.0f, -1.0f, -1.0f, 0xffffff00 },
	{ 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t s_cubeTriList[] =
{
	0, 1, 2, // 0
	1, 3, 2,
	4, 6, 5, // 2
	5, 6, 7,
	0, 2, 4, // 4
	4, 2, 6,
	1, 5, 3, // 6
	5, 7, 3,
	0, 4, 1, // 8
	4, 5, 1,
	2, 3, 6, // 10
	6, 3, 7,
};

static const uint16_t s_cubeTriStrip[] =
{
	0, 1, 2,
	3,
	7,
	1,
	5,
	0,
	4,
	2,
	6,
	7,
	4,
	5,
};

static const uint16_t s_cubeLineList[] =
{
	0, 1,
	0, 2,
	0, 4,
	1, 3,
	1, 5,
	2, 3,
	2, 6,
	3, 7,
	4, 5,
	4, 6,
	5, 7,
	6, 7,
};

static const uint16_t s_cubeLineStrip[] =
{
	0, 2, 3, 1, 5, 7, 6, 4,
	0, 2, 6, 4, 5, 7, 3, 1,
	0,
};

static const uint16_t s_cubePoints[] =
{
	0, 1, 2, 3, 4, 5, 6, 7
};

static const char* s_ptNames[]
{
	"Triangle List",
	"Triangle Strip",
	"Lines",
	"Line Strip",
	"Points",
};

static const uint64_t s_ptState[]
{
	UINT64_C(0),
	BGFX_STATE_PT_TRISTRIP,
	BGFX_STATE_PT_LINES,
	BGFX_STATE_PT_LINESTRIP,
	BGFX_STATE_PT_POINTS,
};
BX_STATIC_ASSERT(BX_COUNTOF(s_ptState) == BX_COUNTOF(s_ptNames) );

class ExampleCubes : public entry::AppI
{
public:
	ExampleCubes(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
		, m_pt(0)
		, m_r(true)
		, m_g(true)
		, m_b(true)
		, m_a(true)
	{
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_NONE;
        // removing BGFX_RESET_MSAA_X4 = enable rendering
        m_reset  = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4;

		bgfx::Init init;
		init.type     = args.m_type;
		init.vendorId = args.m_pciId;
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		

		// Create vertex stream declaration.
		PosColorVertex::init();

		// Create static vertex buffer.
		m_vbh = bgfx::createVertexBuffer(
			// Static data can be passed with bgfx::makeRef
			  bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices) )
			, PosColorVertex::ms_layout
			);

		// Create static index buffer for triangle list rendering.
		m_ibh[0] = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList) )
			);

		// Create static index buffer for triangle strip rendering.
		m_ibh[1] = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeTriStrip, sizeof(s_cubeTriStrip) )
			);

		// Create static index buffer for line list rendering.
		m_ibh[2] = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeLineList, sizeof(s_cubeLineList) )
			);

		// Create static index buffer for line strip rendering.
		m_ibh[3] = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeLineStrip, sizeof(s_cubeLineStrip) )
			);

		// Create static index buffer for point list rendering.
		m_ibh[4] = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubePoints, sizeof(s_cubePoints) )
			);

		// Create program from shaders.
		m_program = loadProgram("vs_cubes", "fs_cubes");

        
        auto depthStencilFormat = bgfx::TextureFormat::D32;
        auto format = bgfx::TextureFormat::BGRA8;
        auto generateMips = false;
        
        assert(bgfx::isTextureValid(0, false, 1, format, BGFX_TEXTURE_RT));
        assert(bgfx::isTextureValid(0, false, 1, depthStencilFormat, BGFX_TEXTURE_RT));

        auto width = 512;
        auto height = 512;
        std::array<bgfx::TextureHandle, 2> textures{
            bgfx::createTexture2D(width, height, generateMips, 1, format, BGFX_TEXTURE_RT),
            bgfx::createTexture2D(width, height, false, 1, depthStencilFormat, BGFX_TEXTURE_RT)};
        std::array<bgfx::Attachment, textures.size()> attachments{};
        for (size_t idx = 0; idx < attachments.size(); ++idx)
        {
            attachments[idx].init(textures[idx]);
        }
        fbh = bgfx::createFrameBuffer(static_cast<uint8_t>(attachments.size()), attachments.data(), true);
        
        
        
		m_timeOffset = bx::getHPCounter();

		imguiCreate();
	}

	virtual int shutdown() override
	{
		imguiDestroy();

		// Cleanup.
		for (uint32_t ii = 0; ii < BX_COUNTOF(m_ibh); ++ii)
		{
			bgfx::destroy(m_ibh[ii]);
		}

		bgfx::destroy(m_vbh);
		bgfx::destroy(m_program);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
			float time = (float)( (bx::getHPCounter()-m_timeOffset)/double(bx::getHPFrequency() ) );

			const bx::Vec3 at  = { 0.0f, 0.0f,   00.0f };
			const bx::Vec3 eye = { 0.0f, 0.0f, -5.0f };

			// Set view and projection matrix for view 0.
			
            float view[16];
            bx::mtxLookAt(view, eye, at);
            float proj[16];
            bx::mtxProj(proj, 60.0f, float(m_width)/float(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
            bgfx::setViewTransform(0, view, proj);
            // Set view 0 default viewport.
            bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );
            
            bgfx::setViewClear(0
                , BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
                , 0x303030ff
                , 1.0f
                , 0
                );
			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

			bgfx::IndexBufferHandle ibh = m_ibh[m_pt];
			uint64_t state = 0
				| (m_r ? BGFX_STATE_WRITE_R : 0)
				| (m_g ? BGFX_STATE_WRITE_G : 0)
				| (m_b ? BGFX_STATE_WRITE_B : 0)
				| (m_a ? BGFX_STATE_WRITE_A : 0)
				| BGFX_STATE_WRITE_Z
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_CULL_CW
				| BGFX_STATE_MSAA
				| s_ptState[m_pt]
				;

			// Submit 11x11 cubes.
            float mtx[16];
            bx::mtxRotateXY(mtx, time, time);

            bgfx::setViewClear(1
                , BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
                , 0x303030ff
                , 1.0f
                , 0
                );

            bgfx::setViewRect(1, 0, 0, 512, 512);
            bgfx::setViewFrameBuffer(1, fbh);
            bgfx::setViewTransform(1, view, proj);
            bgfx::setStencil(0);
            bgfx::setTransform(mtx);
            bgfx::setVertexBuffer(0, m_vbh);
            bgfx::setIndexBuffer(ibh);
            bgfx::setState(state);
            bgfx::submit(1, m_program);

            /*
            Adding clear here fixes the issue
            bgfx::setViewClear(2
                , BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
                , 0x303030ff
                , 1.0f
                , 0
                );
*/
            bgfx::touch(2);
            bgfx::setViewFrameBuffer(2, {bgfx::kInvalidHandle});
            bgfx::setViewRect(2, 0, 0, m_width, m_height);
            bgfx::setViewTransform(2, view, proj);
            bgfx::setTransform(mtx);
            bgfx::setVertexBuffer(0, m_vbh);
            bgfx::setIndexBuffer(ibh);
            bgfx::setState(state);
            
            bgfx::submit(2, m_program);
            

			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	entry::MouseState m_mouseState;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh[BX_COUNTOF(s_ptState)];
	bgfx::ProgramHandle m_program;
    bgfx::FrameBufferHandle fbh;
	int64_t m_timeOffset;
	int32_t m_pt;

	bool m_r;
	bool m_g;
	bool m_b;
	bool m_a;
};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	  ExampleCubes
	, "01-cubes"
	, "Rendering simple static mesh."
	, "https://bkaradzic.github.io/bgfx/examples.html#cubes"
	);
