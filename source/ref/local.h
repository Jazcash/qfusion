/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2002-2013 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef R_LOCAL_H
#define R_LOCAL_H

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_cvar.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/bsp.h"
#include "../qcommon/patch.h"

#ifdef ALIGN
#undef ALIGN
#endif

#define ALIGN( x, a ) ( ( ( x ) + ( ( size_t )( a ) - 1 ) ) & ~( ( size_t )( a ) - 1 ) )

typedef struct mempool_s mempool_t;
typedef struct cinematics_s cinematics_t;
typedef struct qthread_s qthread_t;
typedef struct qmutex_s qmutex_t;
typedef struct qbufPipe_s qbufPipe_t;

typedef unsigned short elem_t;

typedef vec_t instancePoint_t[8]; // quaternion for rotation + xyz pos + uniform scale

#define NUM_CUSTOMCOLORS        16

#include "../cgame/ref.h"
#include "refmath.h"
#include "vattribs.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

typedef struct superLightStyle_s {
	vattribmask_t vattribs;
	int lightmapNum[MAX_LIGHTMAPS];
	int lightmapStyles[MAX_LIGHTMAPS];
	int vertexStyles[MAX_LIGHTMAPS];
	float stOffset[MAX_LIGHTMAPS][2];
} superLightStyle_t;

#include "glimp.h"
#include "surface.h"
#include "image.h"
#include "mesh.h"
#include "shader.h"
#include "backend.h"
//#include "shadow.h"
#include "model.h"
#include "trace.h"
#include "program.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef CGAMEGETLIGHTORIGIN
#define SHADOW_MAPPING          2
#else
#define SHADOW_MAPPING          1
#endif

#define SUBDIVISIONS_MIN        3
#define SUBDIVISIONS_MAX        16
#define SUBDIVISIONS_DEFAULT    5

#define MAX_PORTAL_SURFACES     32
#define MAX_PORTAL_TEXTURES     64

#define NUM_BLOOM_LODS          4

#define BACKFACE_EPSILON        4

#define ON_EPSILON              0.1         // point on plane side epsilon

#define Z_NEAR                  4.0f
#define Z_BIAS                  64.0f

#define SIDE_FRONT              0
#define SIDE_BACK               1
#define SIDE_ON                 2

#define RF_BIT( x )               ( 1ULL << ( x ) )

#define RF_NONE                 0x0
#define RF_MIRRORVIEW           RF_BIT( 0 )
#define RF_PORTALVIEW           RF_BIT( 1 )
#define RF_ENVVIEW              RF_BIT( 2 )
#define RF_SHADOWMAPVIEW        RF_BIT( 3 )
#define RF_FLIPFRONTFACE        RF_BIT( 4 )
#define RF_DRAWFLAT             RF_BIT( 5 )
#define RF_CLIPPLANE            RF_BIT( 6 )
#define RF_NOVIS                RF_BIT( 7 )
#define RF_LIGHTMAP             RF_BIT( 8 )
#define RF_SOFT_PARTICLES       RF_BIT( 9 )
#define RF_PORTAL_CAPTURE       RF_BIT( 10 )
#define RF_SHADOWMAPVIEW_RGB    RF_BIT( 11 )

#define RF_CUBEMAPVIEW          ( RF_ENVVIEW )
#define RF_NONVIEWERREF         ( RF_PORTALVIEW | RF_MIRRORVIEW | RF_ENVVIEW | RF_SHADOWMAPVIEW )

#define MAX_REF_SCENES          32 // max scenes rendered per frame
#define MAX_REF_ENTITIES        ( MAX_ENTITIES + 48 ) // must not exceed 2048 because of sort key packing

//===================================================================

typedef struct refScreenTexSet_s {
	image_t         *screenTex;
	image_t         *screenTexCopy;
	image_t         *screenPPCopies[2];
	image_t         *screenDepthTex;
	image_t         *screenDepthTexCopy;
	image_t         *screenOverbrightTex; // the overbrights output target
	image_t         *screenBloomLodTex[NUM_BLOOM_LODS][2]; // lods + backups for bloom
	int multisampleTarget;                // multisample fbo
} refScreenTexSet_t;

typedef struct portalSurface_s {
	const entity_t  *entity;
	cplane_t plane, untransformed_plane;
	const shader_t  *shader;
	vec3_t mins, maxs, centre;
	image_t         *texures[2];            // front and back portalmaps
	skyportal_t     *skyPortal;
} portalSurface_t;

typedef struct {
	unsigned int renderFlags;
	unsigned int dlightBits;

	int renderTarget;                       // target framebuffer object
	bool multisampleDepthResolved;

	int scissor[4];
	int viewport[4];

	//
	// view origin
	//
	vec3_t viewOrigin;
	mat3_t viewAxis;
	cplane_t frustum[6];
	float farClip;
	unsigned int clipFlags;
	vec3_t visMins, visMaxs;
	float visFarClip;
	float hdrExposure;

	vec3_t lodOrigin;
	vec3_t pvsOrigin;
	cplane_t clipPlane;

	mat4_t objectMatrix;
	mat4_t cameraMatrix;

	mat4_t modelviewMatrix;
	mat4_t projectionMatrix;

	mat4_t cameraProjectionMatrix;                  // cameraMatrix * projectionMatrix
	mat4_t modelviewProjectionMatrix;               // modelviewMatrix * projectionMatrix

	drawSurfaceSky_t skyDrawSurface;

	float lod_dist_scale_for_fov;

	unsigned int numPortalSurfaces;
	unsigned int numDepthPortalSurfaces;
	portalSurface_t portalSurfaces[MAX_PORTAL_SURFACES];
	portalSurface_t *skyportalSurface;

	refdef_t refdef;

	refScreenTexSet_t *st;                  // points to either either a 8bit or a 16bit float set

	drawList_t      *meshlist;              // meshes to be rendered
	drawList_t      *portalmasklist;        // sky and portal BSP surfaces are rendered before (sky-)portals
											// to create depth mask
	mfog_t          *fog_eye;
} refinst_t;

//====================================================

enum class BuiltinTexNumber {
	Raw,
	RawYuv0,
	No = RawYuv0 + 3,
	White,
	WhiteCubemap,
	Black,
	Grey,
	BlankBump,
	Particle,
	Corona,
	Portal0
};

// globals shared by the frontend and the backend
// the backend should never attempt modifying any of these
typedef struct {
	// any asset (model, shader, texture, etc) with has not been registered
	// or "touched" during the last registration sequence will be freed
	volatile int registrationSequence;
	volatile bool registrationOpen;

	// bumped each time R_RegisterWorldModel is called
	volatile int worldModelSequence;

	float sinTableByte[256];

	model_t         *worldModel;
	mbrushmodel_t   *worldBrushModel;

	struct mesh_vbo_s *nullVBO;
	struct mesh_vbo_s *postProcessingVBO;

	vec3_t wallColor, floorColor;

	image_t         *rawTexture;                // cinematic texture (RGB)
	image_t         *rawYUVTextures[3];         // 8bit cinematic textures (YCbCr)
	image_t         *noTexture;                 // use for bad textures
	image_t         *whiteTexture;
	image_t         *whiteCubemapTexture;
	image_t         *blackTexture;
	image_t         *greyTexture;
	image_t         *blankBumpTexture;
	image_t         *particleTexture;           // little dot for particles
	image_t         *coronaTexture;
	image_t         *portalTextures[MAX_PORTAL_TEXTURES + 1];

	refScreenTexSet_t st, stf;

	shader_t        *envShader;
	shader_t        *skyShader;
	shader_t        *whiteShader;
	shader_t        *emptyFogShader;

	byte_vec4_t customColors[NUM_CUSTOMCOLORS];
} r_shared_t;

typedef struct {
	// bumped each R_ClearScene
	unsigned int frameCount;

	unsigned int numEntities;
	unsigned int numLocalEntities;
	entity_t entities[MAX_REF_ENTITIES];
	entity_t        *worldent;
	entity_t        *polyent;
	entity_t        *skyent;

	unsigned int numPolys;
	drawSurfacePoly_t polys[MAX_POLYS];

	lightstyle_t lightStyles[MAX_LIGHTSTYLES];

	unsigned int numBmodelEntities;
	entity_t        *bmodelEntities[MAX_REF_ENTITIES];

	float farClipMin, farClipBias;

	refdef_t refdef;
} r_scene_t;

class Scene {
	template <typename> friend class SingletonHolder;

	shader_t *coronaShader { nullptr };

	Scene() {
		for( int i = 0; i < MAX_CORONA_LIGHTS; ++i ) {
			coronaSurfs[i] = ST_CORONA;
		}
	}
public:
	using LightNumType = uint8_t;

	class Light {
		friend class Scene;
	public:
		vec3_t color;
		float radius;
		vec4_t center;
	private:
		Light() {}

		Light( const float *center_, const float *color_, float radius_ ) {
			VectorCopy( color_, this->color );
			this->radius = radius_;
			VectorCopy( center_, this->center );
			this->center[3] = 0;
		}
	};

private:
	enum { MAX_CORONA_LIGHTS = 255 };
	enum { MAX_PROGRAM_LIGHTS = 255 };

	Light coronaLights[MAX_CORONA_LIGHTS];
	Light programLights[MAX_PROGRAM_LIGHTS];

	int numCoronaLights { 0 };
	int numProgramLights { 0 };

	LightNumType drawnCoronaLightNums[MAX_CORONA_LIGHTS];
	LightNumType drawnProgramLightNums[MAX_PROGRAM_LIGHTS];

	int numDrawnCoronaLights { 0 };
	int numDrawnProgramLights { 0 };

	drawSurfaceType_t coronaSurfs[MAX_CORONA_LIGHTS];

	uint32_t BitsForNumberOfLights( int numLights ) {
		assert( numLights <= 32 );
		return (uint32_t)( ( 1ull << (uint64_t)( numLights ) ) - 1 );
	}
public:
	static void Init();
	static void Shutdown();
	static Scene *Instance();

	void Clear() {
		numCoronaLights = 0;
		numProgramLights = 0;

		numDrawnCoronaLights = 0;
		numDrawnProgramLights = 0;
	}

	void InitVolatileAssets();

	void DestroyVolatileAssets();

	void AddLight( const vec3_t origin, float programIntensity, float coronaIntensity, float r, float g, float b );

	void DynLightDirForOrigin( const vec3_t origin, float radius, vec3_t dir, vec3_t diffuseLocal, vec3_t ambientLocal );

	const Light *LightForCoronaSurf( const drawSurfaceType_t *surf ) const {
		return &coronaLights[surf - coronaSurfs];
	}

	// CAUTION: The meaning of dlight bits has been changed:
	// a bit correspond to an index of a light num in drawnProgramLightNums
	uint32_t CullLights( unsigned clipFlags );

	void DrawCoronae();

	const Light *ProgramLightForNum( LightNumType num ) const {
		assert( (unsigned)num < (unsigned)MAX_PROGRAM_LIGHTS );
		return &programLights[num];
	}

	void GetDrawnProgramLightNums( const LightNumType **rangeBegin, const LightNumType **rangeEnd ) const {
		*rangeBegin = drawnProgramLightNums;
		*rangeEnd = drawnProgramLightNums + numDrawnProgramLights;
	}
};

// global frontend variables are stored here
// the backend should never attempt reading or modifying them
typedef struct {
	bool in2D;
	int width2D, height2D;

	int frameBufferWidth, frameBufferHeight;

	int swapInterval;

	int worldModelSequence;

	// used for dlight push checking
	unsigned int frameCount;

	int viewcluster, viewarea;

	struct {
		unsigned int c_brush_polys, c_world_leafs;
		unsigned int c_slices_verts, c_slices_elems;
		unsigned int c_world_draw_surfs;
		unsigned int t_cull_world_nodes, t_cull_world_surfs;
		unsigned int t_world_node;
		unsigned int t_add_world_surfs;
		unsigned int t_add_polys, t_add_entities;
		unsigned int t_draw_meshes;
	} stats;

	struct {
		unsigned average;        // updates 4 times per second
		int64_t time, oldTime;
		unsigned count, oldCount;
	} frameTime;

	volatile bool dataSync;   // call R_Finish

	char speedsMsg[2048];
	qmutex_t        *speedsMsgLock;

	msurface_t      *debugSurface;
	qmutex_t        *debugSurfaceLock;

	unsigned int numWorldSurfVis;
	volatile unsigned char *worldSurfVis;
	volatile unsigned char *worldSurfFullVis;

	unsigned int numWorldLeafVis;
	volatile unsigned char *worldLeafVis;

	unsigned int numWorldDrawSurfVis;
	volatile unsigned char *worldDrawSurfVis;

	char drawBuffer[32];
	bool newDrawBuffer;
} r_globals_t;

extern r_shared_t rsh;
extern r_scene_t rsc;
extern r_globals_t rf;

#define R_ENT2NUM( ent ) ( ( ent ) - rsc.entities )
#define R_NUM2ENT( num ) ( rsc.entities + ( num ) )

extern cvar_t *r_norefresh;
extern cvar_t *r_drawentities;
extern cvar_t *r_drawworld;
extern cvar_t *r_speeds;
extern cvar_t *r_drawelements;
extern cvar_t *r_fullbright;
extern cvar_t *r_lightmap;
extern cvar_t *r_novis;
extern cvar_t *r_nocull;
extern cvar_t *r_lerpmodels;
extern cvar_t *r_brightness;
extern cvar_t *r_sRGB;

extern cvar_t *r_dynamiclight;
extern cvar_t *r_detailtextures;
extern cvar_t *r_subdivisions;
extern cvar_t *r_showtris;
extern cvar_t *r_shownormals;
extern cvar_t *r_draworder;
extern cvar_t *r_leafvis;

extern cvar_t *r_fastsky;
extern cvar_t *r_portalonly;
extern cvar_t *r_portalmaps;
extern cvar_t *r_portalmaps_maxtexsize;

extern cvar_t *r_lighting_deluxemapping;
extern cvar_t *r_lighting_specular;
extern cvar_t *r_lighting_glossintensity;
extern cvar_t *r_lighting_glossexponent;
extern cvar_t *r_lighting_ambientscale;
extern cvar_t *r_lighting_directedscale;
extern cvar_t *r_lighting_packlightmaps;
extern cvar_t *r_lighting_maxlmblocksize;
extern cvar_t *r_lighting_vertexlight;
extern cvar_t *r_lighting_maxglsldlights;
extern cvar_t *r_lighting_grayscale;
extern cvar_t *r_lighting_intensity;

extern cvar_t *r_offsetmapping;
extern cvar_t *r_offsetmapping_scale;
extern cvar_t *r_offsetmapping_reliefmapping;

extern cvar_t *r_shadows;
extern cvar_t *r_shadows_alpha;
extern cvar_t *r_shadows_nudge;
extern cvar_t *r_shadows_projection_distance;
extern cvar_t *r_shadows_maxtexsize;
extern cvar_t *r_shadows_pcf;
extern cvar_t *r_shadows_self_shadow;
extern cvar_t *r_shadows_dither;

extern cvar_t *r_outlines_world;
extern cvar_t *r_outlines_scale;
extern cvar_t *r_outlines_cutoff;

extern cvar_t *r_soft_particles;
extern cvar_t *r_soft_particles_scale;

extern cvar_t *r_hdr;
extern cvar_t *r_hdr_gamma;
extern cvar_t *r_hdr_exposure;

extern cvar_t *r_bloom;

extern cvar_t *r_fxaa;
extern cvar_t *r_samples;

extern cvar_t *r_lodbias;
extern cvar_t *r_lodscale;

extern cvar_t *r_gamma;
extern cvar_t *r_texturemode;
extern cvar_t *r_texturefilter;
extern cvar_t *r_texturecompression;
extern cvar_t *r_mode;
extern cvar_t *r_nobind;
extern cvar_t *r_picmip;
extern cvar_t *r_skymip;
extern cvar_t *r_polyblend;
extern cvar_t *r_lockpvs;
extern cvar_t *r_screenshot_fmtstr;
extern cvar_t *r_screenshot_jpeg;
extern cvar_t *r_screenshot_jpeg_quality;
extern cvar_t *r_swapinterval;
extern cvar_t *r_swapinterval_min;

extern cvar_t *r_temp1;

extern cvar_t *r_drawflat;
extern cvar_t *r_wallcolor;
extern cvar_t *r_floorcolor;

extern cvar_t *r_usenotexture;

extern cvar_t *r_maxglslbones;

extern cvar_t *r_multithreading;

extern cvar_t *r_showShaderCache;

extern cvar_t *gl_cull;

extern cvar_t *vid_displayfrequency;
extern cvar_t *vid_multiscreen_head;

//====================================================================

void R_NormToLatLong( const vec_t *normal, uint8_t latlong[2] );
void R_LatLongToNorm( const uint8_t latlong[2], vec3_t out );
void R_LatLongToNorm4( const uint8_t latlong[2], vec4_t out );

#define R_LinearFloatFromsRGBFloat( c ) ( ( ( c ) <= 0.04045f ) ? ( c ) * ( 1.0f / 12.92f ) : (float)pow( ( ( c ) + 0.055f ) * ( 1.0f / 1.055f ), 2.4f ) )
#define R_sRGBFloatFromLinearFloat( c ) ( ( ( c ) < 0.0031308f ) ? ( c ) * 12.92f : 1.055f * (float)pow( ( c ), 1.0f / 2.4f ) - 0.055f )
#define R_LinearFloatFromsRGB( c ) Image_LinearFloatFromsRGBFloat( ( c ) * ( 1.0f / 255.0f ) )
#define R_sRGBFloatFromLinear( c ) Image_sRGBFloatFromLinearFloat( ( c ) * ( 1.0f / 255.0f ) )

//====================================================================

//
// r_alias.c
//
bool    R_AddAliasModelToDrawList( const entity_t *e );
void    R_DrawAliasSurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, const portalSurface_t *portalSurface, unsigned int shadowBits, drawSurfaceAlias_t *drawSurf );
bool    R_AliasModelLerpTag( orientation_t *orient, const maliasmodel_t *aliasmodel, int framenum, int oldframenum,
							 float lerpfrac, const char *name );
void        R_AliasModelFrameBounds( const model_t *mod, int frame, vec3_t mins, vec3_t maxs );

//
// r_cmds.c
//
void        R_TakeScreenShot( const char *path, const char *name, const char *fmtString, int x, int y, int w, int h, bool silent );
void        R_ScreenShot_f( void );
void        R_TakeEnvShot( const char *path, const char *name, unsigned maxPixels );
void        R_EnvShot_f( void );
void        R_ImageList_f( void );

//
// r_cull.c
//
void        R_SetupFrustum( const refdef_t *rd, float farClip, cplane_t *frustum );
bool    R_CullBox( const vec3_t mins, const vec3_t maxs, const unsigned int clipflags );
bool    R_CullSphere( const vec3_t centre, const float radius, const unsigned int clipflags );
bool    R_VisCullBox( const vec3_t mins, const vec3_t maxs );
bool    R_VisCullSphere( const vec3_t origin, float radius );
int         R_CullModelEntity( const entity_t *e, vec3_t mins, vec3_t maxs, float radius, bool sphereCull, bool pvsCull );
bool    R_CullSpriteEntity( const entity_t *e );

//
// r_framebuffer.c
//
enum {
	FBO_COPY_NORMAL = 0,
	FBO_COPY_CENTREPOS = 1,
	FBO_COPY_INVERT_Y = 2,
	FBO_COPY_NORMAL_DST_SIZE = 3,
};

void        RFB_Init( void );
int         RFB_RegisterObject( int width, int height, bool builtin, bool depthRB, bool stencilRB, bool colorRB, int samples, bool useFloat );
void        RFB_UnregisterObject( int object );
void        RFB_TouchObject( int object );
void        RFB_BindObject( int object );
int         RFB_BoundObject( void );
bool        RFB_AttachTextureToObject( int object, bool depth, int target, image_t *texture );
image_t     *RFB_GetObjectTextureAttachment( int object, bool depth, int target );
bool        RFB_HasColorRenderBuffer( int object );
bool        RFB_HasDepthRenderBuffer( int object );
bool        RFB_HasStencilRenderBuffer( int object );
int         RFB_GetSamples( int object );
void        RFB_BlitObject( int src, int dest, int bitMask, int mode, int filter, int readAtt, int drawAtt );
bool        RFB_CheckObjectStatus( void );
void        RFB_GetObjectSize( int object, int *width, int *height );
void        RFB_FreeUnusedObjects( void );
void        RFB_Shutdown( void );

//
// r_light.c
//
#define MAX_SUPER_STYLES    128

unsigned int R_AddSurfaceDlighbits( const msurface_t *surf, unsigned int checkDlightBits );
void        R_AddDynamicLights( unsigned int dlightbits, int state );
void        R_LightForOrigin( const vec3_t origin, vec3_t dir, vec4_t ambient, vec4_t diffuse, float radius, bool noWorldLight );
float       R_LightExposureForOrigin( const vec3_t origin );
void        R_BuildLightmaps( model_t *mod, int numLightmaps, int w, int h, const uint8_t *data, mlightmapRect_t *rects );
void        R_InitLightStyles( model_t *mod );
superLightStyle_t   *R_AddSuperLightStyle( model_t *mod, const int *lightmaps,
										   const uint8_t *lightmapStyles, const uint8_t *vertexStyles, mlightmapRect_t **lmRects );
void        R_SortSuperLightStyles( model_t *mod );
void        R_TouchLightmapImages( model_t *mod );

void        R_BatchCoronaSurf(  const entity_t *e, const shader_t *shader, const mfog_t *fog, const portalSurface_t *portalSurface, unsigned int shadowBits, drawSurfaceType_t *drawSurf );

//
// r_main.c
//
#define R_FASTSKY() ( r_fastsky->integer || rf.viewcluster == -1 || mapConfig.skipSky )

extern mempool_t *r_mempool;

#ifndef MEMPOOL_REFMODULE
#define MEMPOOL_REFMODULE ( 256 )
#endif

ATTRIBUTE_MALLOC void *_Mem_AllocExt( mempool_t *pool, size_t size, size_t aligment, int z, int musthave, int canthave, const char *filename, int fileline );
ATTRIBUTE_MALLOC void *_Mem_Alloc( mempool_t *pool, size_t size, int musthave, int canthave, const char *filename, int fileline );
void *_Mem_Realloc( void *data, size_t size, const char *filename, int fileline );
void _Mem_Free( void *data, int musthave, int canthave, const char *filename, int fileline );
mempool_t *_Mem_AllocPool( mempool_t *parent, const char *name, int flags, const char *filename, int fileline );
mempool_t *_Mem_AllocTempPool( const char *name, const char *filename, int fileline );
void _Mem_FreePool( mempool_t **pool, int musthave, int canthave, const char *filename, int fileline );
void _Mem_EmptyPool( mempool_t *pool, int musthave, int canthave, const char *filename, int fileline );
char *_Mem_CopyString( mempool_t *pool, const char *in, const char *filename, int fileline );

#define R_Malloc( size ) R_Malloc_( size, __FILE__, __LINE__ )
#define R_Realloc( data, size ) _Mem_Realloc( data, size, __FILE__, __LINE__ )
#define R_Free( data ) _Mem_Free( data, MEMPOOL_REFMODULE, 0, __FILE__, __LINE__ )
#define R_AllocPool( parent, name ) _Mem_AllocPool( parent, name, MEMPOOL_REFMODULE, __FILE__, __LINE__ )
#define R_FreePool( pool ) _Mem_FreePool( pool, MEMPOOL_REFMODULE, 0, __FILE__, __LINE__ )
#define R_MallocExt( pool,size,align,z ) _Mem_AllocExt( pool,size,align,z,MEMPOOL_REFMODULE,0,__FILE__,__LINE__ )

ATTRIBUTE_MALLOC void * R_Malloc_( size_t size, const char *filename, int fileline );
char        *R_CopyString_( const char *in, const char *filename, int fileline );
#define     R_CopyString( in ) R_CopyString_( in,__FILE__,__LINE__ )

int         R_LoadFile_( const char *path, int flags, void **buffer, const char *filename, int fileline );
void        R_FreeFile_( void *buffer, const char *filename, int fileline );

#define     R_LoadFile( path,buffer ) R_LoadFile_( path,0,buffer,__FILE__,__LINE__ )
#define     R_LoadCacheFile( path,buffer ) R_LoadFile_( path,FS_CACHE,buffer,__FILE__,__LINE__ )
#define     R_FreeFile( buffer ) R_FreeFile_( buffer,__FILE__,__LINE__ )

bool        R_IsRenderingToScreen( void );
void        R_BeginFrame( bool forceClear, int swapInterval );
void        R_EndFrame( void );
int         R_SetSwapInterval( int swapInterval, int oldSwapInterval );
void        R_SetGamma( float gamma );
void        R_SetWallFloorColors( const vec3_t wallColor, const vec3_t floorColor );
void        R_Set2DMode( bool enable );
void        R_RenderView( const refdef_t *fd );
const msurface_t *R_GetDebugSurface( void );
const char *R_WriteSpeedsMessage( char *out, size_t size );
void        R_RenderDebugSurface( const refdef_t *fd );
void        R_Finish( void );
void        R_Flush( void );

/**
 * Calls R_Finish if data sync was previously deferred.
 */
void        R_DataSync( void );

/**
 * Defer R_DataSync call at the start/end of the next frame.
 */
void        R_DeferDataSync( void );

mfog_t      *R_FogForBounds( const vec3_t mins, const vec3_t maxs );
mfog_t      *R_FogForSphere( const vec3_t centre, const float radius );
bool    R_CompletelyFogged( const mfog_t *fog, vec3_t origin, float radius );
int         R_LODForSphere( const vec3_t origin, float radius );
float       R_DefaultFarClip( void );

void        R_BatchSpriteSurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, const portalSurface_t *portalSurface, unsigned int shadowBits, drawSurfaceType_t *drawSurf );

struct mesh_vbo_s *R_InitNullModelVBO( void );
void    R_DrawNullSurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, const portalSurface_t *portalSurface, unsigned int shadowBits, drawSurfaceType_t *drawSurf );

struct mesh_vbo_s *R_InitPostProcessingVBO( void );

void        R_TransformForWorld( void );
void        R_TransformForEntity( const entity_t *e );
void        R_TranslateForEntity( const entity_t *e );
void        R_TransformBounds( const vec3_t origin, const mat3_t axis, vec3_t mins, vec3_t maxs, vec3_t bbox[8] );

void        R_DrawStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2,
							  const vec4_t color, const shader_t *shader );
void        R_DrawRotatedStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2,
									 float angle, const vec4_t color, const shader_t *shader );
void        R_UploadRawPic( image_t *texture, int cols, int rows, uint8_t *data );
void        R_DrawStretchRawYUVBuiltin( int x, int y, int w, int h, float s1, float t1, float s2, float t2,
										image_t **yuvTextures, int flip );
void        R_DrawStretchRaw( int x, int y, int w, int h, float s1, float t1, float s2, float t2 );
void        R_DrawStretchRawYUV( int x, int y, int w, int h, float s1, float t1, float s2, float t2 );
void        R_DrawStretchQuick( int x, int y, int w, int h, float s1, float t1, float s2, float t2,
								const vec4_t color, int program_type, image_t *image, int blendMask );

void        R_InitCustomColors( void );
void        R_SetCustomColor( int num, int r, int g, int b );
int         R_GetCustomColor( int num );
void        R_ShutdownCustomColors( void );

#define ENTITY_OUTLINE( ent ) ( ( !( rn.renderFlags & RF_MIRRORVIEW ) && ( ( ent )->renderfx & RF_VIEWERMODEL ) ) ? 0 : ( ent )->outlineHeight )

void        R_ClearRefInstStack( void );
bool        R_PushRefInst( void );
void        R_PopRefInst( void );

void        R_BindFrameBufferObject( int object );

void        R_Scissor( int x, int y, int w, int h );
void        R_GetScissor( int *x, int *y, int *w, int *h );
void        R_ResetScissor( void );

//
// r_mesh.c
//
void R_InitDrawList( drawList_t *list );
void R_ClearDrawList( drawList_t *list );
unsigned R_PackOpaqueOrder( const mfog_t *fog, const shader_t *shader, int numLightmaps, bool dlight );
void *R_AddSurfToDrawList( drawList_t *list, const entity_t *e, const mfog_t *fog, const shader_t *shader,
						   float dist, unsigned int order, const portalSurface_t *portalSurf, void *drawSurf );
void R_UpdateDrawSurfDistKey( void *psds, int renderFx, const shader_t *shader, float dist, unsigned order );
portalSurface_t *R_GetDrawListSurfPortal( void *psds );
void R_AddDrawListVBOSlice( drawList_t *list, unsigned int index, unsigned int numVerts, unsigned int numElems,
					unsigned int firstVert, unsigned int firstElem );
vboSlice_t *R_GetDrawListVBOSlice( drawList_t *list, unsigned int index );
void R_GetVBOSliceCounts( drawList_t *list, unsigned *numSliceVerts, unsigned *numSliceElems );

void R_InitDrawLists( void );

void R_SortDrawList( drawList_t *list );
void R_DrawSurfaces( drawList_t *list );
void R_DrawOutlinedSurfaces( drawList_t *list );

void R_CopyOffsetElements( const elem_t *inelems, int numElems, int vertsOffset, elem_t *outelems );
void R_CopyOffsetTriangles( const elem_t *inelems, int numElems, int vertsOffset, elem_t *outelems );
void R_BuildTrifanElements( int vertsOffset, int numVerts, elem_t *elems );
void R_BuildTangentVectors( int numVertexes, vec4_t *xyzArray, vec4_t *normalsArray, vec2_t *stArray,
							int numTris, elem_t *elems, vec4_t *sVectorsArray );

//
// r_portals.c
//
extern drawList_t r_portallist, r_skyportallist;

portalSurface_t *R_AddPortalSurface( const entity_t *ent, const shader_t *shader, void *drawSurf );
portalSurface_t *R_AddSkyportalSurface( const entity_t *ent, const shader_t *shader, void *drawSurf );
void R_UpdatePortalSurface( portalSurface_t *portalSurface, const mesh_t *mesh,
									 const vec3_t mins, const vec3_t maxs, const shader_t *shader, void *drawSurf );
void R_DrawPortals( void );

//
// r_poly.c
//
void        R_BatchPolySurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, const portalSurface_t *portalSurface, unsigned int shadowBits, drawSurfacePoly_t *poly );
void        R_DrawPolys( void );
void        R_DrawStretchPoly( const poly_t *poly, float x_offset, float y_offset );
bool    R_SurfPotentiallyFragmented( const msurface_t *surf );
int         R_GetClippedFragments( const vec3_t origin, float radius, vec3_t axis[3], int maxfverts,
								   vec4_t *fverts, int maxfragments, fragment_t *fragments );

//
// r_register.c
//
rserr_t     R_Init( const char *applicationName, const char *screenshotPrefix, int startupColor,
					int iconResource, const int *iconXPM,
					void *hinstance, void *wndproc, void *parenthWnd,
					bool verbose );
void        R_BeginRegistration( void );
void        R_EndRegistration( void );
void        R_Shutdown( bool verbose );
rserr_t     R_SetMode( int x, int y, int width, int height, int displayFrequency, bool fullScreen, bool borderless );

//
// r_scene.c
//
extern drawList_t r_worldlist, r_portalmasklist;

void R_AddDebugBounds( const vec3_t mins, const vec3_t maxs, const byte_vec4_t color );
void R_ClearScene( void );
void R_AddEntityToScene( const entity_t *ent );
void R_AddPolyToScene( const poly_t *poly );
void R_AddLightStyleToScene( int style, float r, float g, float b );
void R_RenderScene( const refdef_t *fd );
void R_BlurScreen( void );

//
// r_surf.c
//
#define MAX_SURF_QUERIES        0x1E0

void        R_DrawWorld( void );
bool    R_SurfPotentiallyVisible( const msurface_t *surf );
bool    R_SurfPotentiallyShadowed( const msurface_t *surf );
bool    R_SurfPotentiallyLit( const msurface_t *surf );
bool    R_AddBrushModelToDrawList( const entity_t *e );
float       R_BrushModelBBox( const entity_t *e, vec3_t mins, vec3_t maxs, bool *rotated );
void    R_DrawBSPSurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, const portalSurface_t *portalSurface, unsigned int shadowBits, drawSurfaceBSP_t *drawSurf );

//
// r_skin.c
//
void        R_InitSkinFiles( void );
void        R_ShutdownSkinFiles( void );
void        R_FreeUnusedSkinFiles( void );
struct skinfile_s *R_SkinFile_Load( const char *name );
struct skinfile_s *R_RegisterSkinFile( const char *name );
shader_t    *R_FindShaderForSkinFile( const struct skinfile_s *skinfile, const char *meshname );

//
// r_skm.c
//
bool    R_AddSkeletalModelToDrawList( const entity_t *e );
void    R_DrawSkeletalSurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, const portalSurface_t *portalSurface, unsigned int shadowBits, drawSurfaceSkeletal_t *drawSurf );
float       R_SkeletalModelBBox( const entity_t *e, vec3_t mins, vec3_t maxs );
void        R_SkeletalModelFrameBounds( const model_t *mod, int frame, vec3_t mins, vec3_t maxs );
int         R_SkeletalGetBoneInfo( const model_t *mod, int bonenum, char *name, size_t name_size, int *flags );
void        R_SkeletalGetBonePose( const model_t *mod, int bonenum, int frame, bonepose_t *bonepose );
int         R_SkeletalGetNumBones( const model_t *mod, int *numFrames );
bool        R_SkeletalModelLerpTag( orientation_t *orient, const mskmodel_t *skmodel, int oldframenum, int framenum, float lerpfrac, const char *name );

void        R_InitSkeletalCache( void );
void        R_ClearSkeletalCache( void );
void        R_ShutdownSkeletalCache( void );

//
// r_vbo.c
//

typedef enum {
	VBO_TAG_NONE,
	VBO_TAG_WORLD,
	VBO_TAG_MODEL,
	VBO_TAG_STREAM
} vbo_tag_t;

typedef struct mesh_vbo_s {
	unsigned int index;
	int registrationSequence;
	vbo_tag_t tag;

	unsigned int vertexId;
	unsigned int elemId;
	void                *owner;
	unsigned int visframe;

	unsigned int numVerts;
	unsigned int numElems;

	size_t vertexSize;
	size_t arrayBufferSize;
	size_t elemBufferSize;

	vattribmask_t vertexAttribs;
	vattribmask_t halfFloatAttribs;

	size_t normalsOffset;
	size_t sVectorsOffset;
	size_t stOffset;
	size_t lmstOffset[( MAX_LIGHTMAPS + 1 ) / 2];
	size_t lmstSize[( MAX_LIGHTMAPS + 1 ) / 2];
	size_t lmlayersOffset[( MAX_LIGHTMAPS + 3 ) / 4];
	size_t colorsOffset[MAX_LIGHTMAPS];
	size_t bonesIndicesOffset;
	size_t bonesWeightsOffset;
	size_t spritePointsOffset;              // autosprite or autosprite2 centre + radius
	size_t instancesOffset;
} mesh_vbo_t;

void        R_InitVBO( void );
mesh_vbo_t *R_CreateMeshVBO( void *owner, int numVerts, int numElems, int numInstances,
							 vattribmask_t vattribs, vbo_tag_t tag, vattribmask_t halfFloatVattribs );
void        R_ReleaseMeshVBO( mesh_vbo_t *vbo );
void        R_TouchMeshVBO( mesh_vbo_t *vbo );
mesh_vbo_t *R_GetVBOByIndex( int index );
int         R_GetNumberOfActiveVBOs( void );
vattribmask_t R_FillVBOVertexDataBuffer( mesh_vbo_t *vbo, vattribmask_t vattribs, const mesh_t *mesh, void *outData );
void        R_UploadVBOVertexRawData( mesh_vbo_t *vbo, int vertsOffset, int numVerts, const void *data );
vattribmask_t R_UploadVBOVertexData( mesh_vbo_t *vbo, int vertsOffset, vattribmask_t vattribs, const mesh_t *mesh );
void        R_UploadVBOElemData( mesh_vbo_t *vbo, int vertsOffset, int elemsOffset, const mesh_t *mesh );
vattribmask_t R_UploadVBOInstancesData( mesh_vbo_t *vbo, int instOffset, int numInstances, instancePoint_t *instances );
void        R_FreeVBOsByTag( vbo_tag_t tag );
void        R_FreeUnusedVBOs( void );
void        R_ShutdownVBO( void );

//
// r_sky.c
//

enum {
	SKYBOX_RIGHT,
	SKYBOX_LEFT,
	SKYBOX_FRONT,
	SKYBOX_BACK,
	SKYBOX_TOP,
	SKYBOX_BOTTOM       // not used for skydome, but is used for skybox
};

struct skydome_s *R_CreateSkydome( model_t *model );
void        R_TouchSkydome( struct skydome_s *skydome );
void        R_DrawSkySurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, const portalSurface_t *portalSurface, 
	                       unsigned int shadowBits, drawSurfaceSky_t *drawSurf );
void        R_ClearSky( drawSurfaceSky_t *drawSurf );

/**
* Maps world surface to skybox side
*
* @param fa world surface
* @return returns true if surface has been successfully mapped to skybox axis
*/
bool        R_ClipSkySurface( drawSurfaceSky_t *drawSurf, const msurface_t *fa );
void        *R_AddSkySurfToDrawList( drawList_t *list, const shader_t *shader, const portalSurface_t *portalSurf, 
	                                 drawSurfaceSky_t *drawSurf );

//====================================================================

typedef struct {
	int overbrightBits;                     // map specific overbright bits

	float ambient[3];
	byte_vec4_t outlineColor;
	byte_vec4_t environmentColor;
	float averageLightingIntensity;

	bool lightmapsPacking;
	bool lightmapArrays;                    // true if using array textures for lightmaps
	int maxLightmapSize;                    // biggest dimension of the largest lightmap
	bool deluxeMaps;                        // true if there are valid deluxemaps in the .bsp
	bool deluxeMappingEnabled;              // true if deluxeMaps is true and r_lighting_deluxemaps->integer != 0

	bool forceClear;

	bool skipSky;

	bool forceWorldOutlines;
} mapconfig_t;

extern mapconfig_t mapConfig;
extern refinst_t rn;

#endif // R_LOCAL_H
