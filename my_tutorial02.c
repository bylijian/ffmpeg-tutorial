#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <SDL2/SDL.h>

#include <stdio.h>

int main(int argc, char *argv[]) {
  AVFormatContext *pFormatCtx = NULL;
  int             i, videoStream;
  AVCodecContext  *pCodecCtx = NULL;
  AVCodec         *pCodec = NULL;
  AVFrame         *pFrame = NULL; 
  AVFrame         *pYUVFrame = NULL;

  AVPacket        packet;
  int             frameFinished,numBytes;
  uint8_t *       buffer;
  //float           aspect_ratio;

  AVDictionary    *optionsDict = NULL;
  struct SwsContext *sws_ctx = NULL;

  /**sdl1.2 @deprecated 
  SDL_Overlay     *bmp = NULL;
  SDL_Surface     *screen = NULL;
  SDL_Rect        rect;
  SDL_Event       event;
  **/

  SDL_Window *screen; 
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Rect sdlRect;
	int screen_w=0;
  int screen_h=0;

  if(argc < 2) {
    fprintf(stderr, "Usage: test <file>\n");
    exit(1);
  }

  // Register all formats and codecs
  av_register_all();
  
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
    exit(1);
  }

  // Open video file
  if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
    return -1; // Couldn't open file
  
  // Retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  av_dump_format(pFormatCtx, 0, argv[1], 0);
  
  // Find the first video stream
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++){
    if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
	}
  if(videoStream==-1)
    return -1; // Didn't find a video stream
  /**
  // Get a pointer to the codec context for the video stream
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;
  
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  **/
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);

  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1; // Codec not found
  }
  pCodecCtx=avcodec_alloc_context3(pCodec);
  
  if(pCodecCtx==NULL) {
    fprintf(stderr, "Unsupported codecContext!\n");
    return -1; // Codec not found
  }
  
  avcodec_parameters_to_context(pCodecCtx,pFormatCtx->streams[videoStream]->codecpar);

  // Open codec
  if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
    return -1; // Could not open codec
  
  // Allocate video frame but not alloc it's buffer
  pFrame=av_frame_alloc();
  pYUVFrame=av_frame_alloc();

  //pYUVFrame is used to provide YUV data to SDL,we must alloc it's buffer ourselves.
  numBytes=av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
			      pCodecCtx->height,1);

  buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	av_image_fill_arrays(pYUVFrame->data, pYUVFrame->linesize,
                         buffer, AV_PIX_FMT_YUV420P,pCodecCtx->width,pCodecCtx->height,1);
  // Make a screen to put our video

/**
#ifndef __DARWIN__
        screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
#else
        screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 24, 0);
#endif
  if(!screen) {
    fprintf(stderr, "SDL: could not set video mode - exiting\n");
    exit(1);
  }


  // Allocate a place to put our YUV image on that screen
  bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
				 pCodecCtx->height,
				 SDL_YV12_OVERLAY,
				 screen);
**/


	//init SDL2.0
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO )) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	} 
	//SDL 2.0 Support for multiple windows
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h,SDL_WINDOW_OPENGL);

	if(!screen) {  
		printf("SDL: could not create window - exiting:%s\n",SDL_GetError());  
		return -1;
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);  
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,pCodecCtx->width,pCodecCtx->height);  

	sdlRect.x=0;
	sdlRect.y=0;
	sdlRect.w=screen_w;
	sdlRect.h=screen_h;

  sws_ctx =
    sws_getContext
    (
        pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

  // Read frames and save first five frames to disk
  i=0;
  while(av_read_frame(pFormatCtx, &packet)>=0) {
    // Is this a packet from the video stream?
    if(packet.stream_index==videoStream) {
      // Decode video frame
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, 
			   &packet);
      
      // Did we get a video frame?
      if(frameFinished) {
  /**
	SDL_LockYUVOverlay(bmp);

	AVPicture pict;
	pict.data[0] = bmp->pixels[0];
	pict.data[1] = bmp->pixels[2];
	pict.data[2] = bmp->pixels[1];

	pict.linesize[0] = bmp->pitches[0];
	pict.linesize[1] = bmp->pitches[2];
	pict.linesize[2] = bmp->pitches[1];

	// Convert the image into YUV format that SDL uses
    sws_scale
    (
        sws_ctx, 
        (uint8_t const * const *)pFrame->data, 
        pFrame->linesize, 
        0,
        pCodecCtx->height,
        pict.data,
        pict.linesize
    );
	
	SDL_UnlockYUVOverlay(bmp);
	
	rect.x = 0;
	rect.y = 0;
	rect.w = pCodecCtx->width;
	rect.h = pCodecCtx->height;
	SDL_DisplayYUVOverlay(bmp, &rect);
  **/
  //Convert the image into YUV format that SDL uses
  sws_scale(sws_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pYUVFrame->data, pYUVFrame->linesize);
	//SDL---------------------------
	SDL_UpdateTexture( sdlTexture, &sdlRect, pYUVFrame->data[0], pYUVFrame->linesize[0] );  
	SDL_RenderClear( sdlRenderer );  
	//SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );  
	SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, NULL);  
	SDL_RenderPresent( sdlRenderer );    

  // Free the packet that was allocated by av_read_frame
  av_packet_unref(&packet);
			}
		}
  }
  
  // Free the YUV frame
  av_free(pFrame);
  
  // Close the codec
  avcodec_close(pCodecCtx);
  
  // Close the video file
  avformat_close_input(&pFormatCtx);
  
  return 0;
}
