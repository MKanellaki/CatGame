#include <SDL2/SDL_image.h>
#include "engine/game.h"
#include "engine/common.h"
#include "engine/sprite.h"
#include "engine/interface.h"
#include "engine/timers.h"
#include "engine/audio.h"

#define WIDTH 640
#define HEIGHT 480
#define FLOOR HEIGHT - 59.0
#define SPEED 480

static struct
{
    ng_game_t game;
    ng_interval_t game_tick;

    // A collection of assets used by entities
    // Ideally, they should have been automatically loaded
    // by iterating over the res/ folder and filling in a hastable
    SDL_Texture *run_texture, *jump_texture, *idle_texture, *sleep_texture;

    ng_animated_sprite_t run, jump, idle, sleep;

    Mix_Chunk *SB_sfx;

    bool is_jumping;
    bool is_running;

} ctx;

static void create_actors(void)
{
    ng_game_create(&ctx.game, "Cat", WIDTH, HEIGHT); //creates window

 
    //We load the animations (run, jump, idle, sleep)
    ctx.run_texture = IMG_LoadTexture(ctx.game.renderer, "assets/cat/run.png");
    ctx.jump_texture = IMG_LoadTexture(ctx.game.renderer, "assets/cat/jump.png");
    ctx.idle_texture = IMG_LoadTexture(ctx.game.renderer, "assets/cat/idle.png");
    ctx.sleep_texture = IMG_LoadTexture(ctx.game.renderer, "assets/cat/sleep.png");

    ng_interval_create(&ctx.game_tick, 50);
    
    //Create animations (run, jump, idle, sleep)
    ng_animated_create(&ctx.run, ctx.run_texture, 7);  //run
    ng_sprite_set_scale(&ctx.run.sprite, 2.0f);
    ctx.run.sprite.transform.x = 100.0f;
    ctx.run.sprite.transform.y = FLOOR;
    
    ng_animated_create(&ctx.jump, ctx.jump_texture, 13);  //jump
    ng_sprite_set_scale(&ctx.jump.sprite, 2.0f);
    ctx.jump.sprite.transform.x = 100.0f;
    ctx.jump.sprite.transform.y = FLOOR;
    
    ng_animated_create(&ctx.idle, ctx.idle_texture, 7);  //idle
    ng_sprite_set_scale(&ctx.idle.sprite, 2.0f);
    ctx.idle.sprite.transform.x = 100.0f;
    ctx.idle.sprite.transform.y = FLOOR;
    
    ng_animated_create(&ctx.sleep, ctx.sleep_texture, 3);  //sleep
    ng_sprite_set_scale(&ctx.sleep.sprite, 2.0f);
    ctx.sleep.sprite.transform.x = 100.0f;
    ctx.sleep.sprite.transform.y = 100.0f;

    //load audio
    ctx.SB_sfx = ng_audio_load("assets/music/OST 1 - Silver Bells (Loopable).wav");


    ctx.is_jumping = false; 
    ctx.is_running = false; 
}

static void handle_event(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:
        if (event->key.keysym.sym == SDLK_SPACE || event->key.keysym.sym == SDLK_UP ) {
            // Only start the jump animation if it's not already jumping
            if (!ctx.is_jumping)
            {
                ctx.is_jumping = true;
                // Reset the jump animation to the first frame
                ng_animated_set_frame(&ctx.jump, 0);
            }
        }
        
        //RUNNING ANIMATION EVENT (when Right Arrow is pressed)
        if (event->key.keysym.sym == SDLK_RIGHT) {
            // Only start the run animation if it's not already running
            if (!ctx.is_running)
            {
                ctx.is_running = true;
                // Reset the run animation to the first frame
                ng_animated_set_frame(&ctx.run, 0);
            }
        }
        break;

    case SDL_KEYUP:
        if (event->key.keysym.sym == SDLK_RIGHT) {
            // Stop running animation when the right key is released
            ctx.is_running = false;
        }
        break;

    }
}

static void update_and_render_scene(float delta)
{
    // Handling "continuous" events, which are now repeatable
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    
    if (keys[SDL_SCANCODE_LEFT]){ //move left
        if (ctx.run.sprite.transform.x > 0) { //wall boundary
            ctx.run.sprite.transform.x -= SPEED * delta; 
            ctx.idle.sprite.transform.x -= SPEED * delta;
            ctx.jump.sprite.transform.x -= SPEED * delta;  
        }
    } 

    if (keys[SDL_SCANCODE_RIGHT]){ //move right
        if (ctx.run.sprite.transform.x < WIDTH - 64){ //wall boundary
            ctx.run.sprite.transform.x += SPEED* delta;
            ctx.jump.sprite.transform.x += SPEED* delta;
            ctx.idle.sprite.transform.x += SPEED* delta;
        }
    }


    if (ctx.is_jumping) {
        // Move to the next frame of the jump animation
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.jump, (ctx.jump.frame + 1) % ctx.jump.total_frames);
        }

        // Check if the jump animation has finished
        if (ctx.jump.frame == ctx.jump.total_frames - 1) {
            ctx.is_jumping = false;  // Animation has finished, stop jumping

            if (keys[SDL_SCANCODE_RIGHT]) { //if right is being pressed continue running after the jump
                ctx.is_running = true;
                ng_animated_set_frame(&ctx.run, 0);  // Reset to the first frame of the run animation
            }
        }
    }else if(ctx.is_running){
        // Move to the next frame of the running animation
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.run, (ctx.run.frame + 1) % ctx.run.total_frames);
        }

    }else{
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.idle, (ctx.idle.frame + 1) % ctx.idle.total_frames);
        }
    }

    // Render animations
    if (ctx.is_jumping) {
        ng_sprite_render(&ctx.jump.sprite, ctx.game.renderer);
    } else if (ctx.is_running){
         ng_sprite_render(&ctx.run.sprite, ctx.game.renderer);
    }else {
        ng_sprite_render(&ctx.idle.sprite, ctx.game.renderer);  // Show idle if not jumping
    }

    ng_audio_play(ctx.SB_sfx);

    
}

int main()
{
    create_actors();
    ng_game_start_loop(&ctx.game,
            handle_event, update_and_render_scene);
}