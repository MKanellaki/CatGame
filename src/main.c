#include <SDL2/SDL_image.h>
#include "engine/game.h"
#include "engine/common.h"
#include "engine/sprite.h"
#include "engine/interface.h"
#include "engine/timers.h"
#include "engine/audio.h"

#define WIDTH 640
#define HEIGHT 640

static struct
{
    ng_game_t game;
    ng_interval_t game_tick;

    // A collection of assets used by entities
    // Ideally, they should have been automatically loaded
    // by iterating over the res/ folder and filling in a hastable
    SDL_Texture *run_texture, *jump_texture, *idle_texture, *sleep_texture;

    ng_animated_sprite_t run, jump, idle, sleep;

    bool is_jumping;
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
    ctx.run.sprite.transform.x = 200.0f;
    
    ng_animated_create(&ctx.jump, ctx.jump_texture, 13);  //jump
    ng_sprite_set_scale(&ctx.jump.sprite, 2.0f);
    ctx.jump.sprite.transform.x = 200.0f;
    
    ng_animated_create(&ctx.idle, ctx.idle_texture, 7);  //idle
    ng_sprite_set_scale(&ctx.idle.sprite, 2.0f);
    ctx.idle.sprite.transform.x = 200.0f;
    
    ng_animated_create(&ctx.sleep, ctx.sleep_texture, 3);  //sleep
    ng_sprite_set_scale(&ctx.sleep.sprite, 2.0f);
    ctx.sleep.sprite.transform.x = 200.0f;

    ctx.is_jumping = false; 
}

static void handle_event(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:
        if (event->key.keysym.sym == SDLK_SPACE) {
            // Only start the jump animation if it's not already jumping
            if (!ctx.is_jumping)
            {
                ctx.is_jumping = true;
                // Reset the jump animation to the first frame
                ng_animated_set_frame(&ctx.jump, 0);
            }
        }
        
        //RUNNING ANIMATION EVENT
        if (event->key.keysym.sym == SDLK_SPACE) {
            // Only start the run animation if it's not already running
            if (!ctx.is_running)
            {
                ctx.is_running = true;
                // Reset the jump animation to the first frame
                ng_animated_set_frame(&ctx.run, 0);
            }
        }
        break;

    }
}

static void update_and_render_scene(float delta)
{
    // Handling "continuous" events, which are now repeatable
    const Uint8* keys = SDL_GetKeyboardState(NULL);


    if (ctx.is_jumping) {
        // Move to the next frame of the jump animation
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.jump, (ctx.jump.frame + 1) % ctx.jump.total_frames);
        }

        // Check if the jump animation has finished
        if (ctx.jump.frame == ctx.jump.total_frames - 1) {
            ctx.is_jumping = false;  // Animation has finished, stop jumping
            // After the jump ends, you could switch to another animation like idle or run
        }
    }else{
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.idle, (ctx.idle.frame + 1) % ctx.idle.total_frames);
        }
    }

    // Render player and animations
    if (ctx.is_jumping) {
        ng_sprite_render(&ctx.jump.sprite, ctx.game.renderer);
    } else {
        ng_sprite_render(&ctx.idle.sprite, ctx.game.renderer);  // Show idle if not jumping
    }

    //FOR RUNNING ANIMATION
    if (ctx.is_running) {
        // Move to the next frame of the running animation
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.run, (ctx.run.frame + 1) % ctx.run.total_frames);
        }

        // Check if the run animation has finished
        if (ctx.run.frame == ctx.run.total_frames - 1) {
            ctx.is_running = false;  // Animation has finished, stop running
            // After the run ends, you could switch to another animation like idle or run
        }
    }else{
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.idle, (ctx.idle.frame + 1) % ctx.idle.total_frames);
        }
    }

    // Render player and animations
    if (ctx.is_running) {
        ng_sprite_render(&ctx.run.sprite, ctx.game.renderer);
    } else {
        ng_sprite_render(&ctx.idle.sprite, ctx.game.renderer);  // Show idle if not running
    }
}

int main()
{
    create_actors();
    ng_game_start_loop(&ctx.game,
            handle_event, update_and_render_scene);
}