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
    Mix_Chunk *gem_sfx;
    TTF_Font *main_font;

    ng_sprite_t player;
    ng_label_t welcome_text;
    ng_animated_sprite_t run, jump, idle, sleep;
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
    
    ng_animated_create(&ctx.idle, ctx.idle_texture, 9);  //idle
    ng_sprite_set_scale(&ctx.idle.sprite, 2.0f);
    ctx.idle.sprite.transform.x = 200.0f;
    
    ng_animated_create(&ctx.sleep, ctx.sleep_texture, 9);  //sleep
    ng_sprite_set_scale(&ctx.sleep.sprite, 2.0f);
    ctx.sleep.sprite.transform.x = 200.0f;


}

static void handle_event(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:
        if (event->key.keysym.sym == SDLK_SPACE){
            
            if (ng_interval_is_ready(&ctx.game_tick))
            {
                ng_animated_set_frame(&ctx.jump, (ctx.jump.frame + 1)
                % ctx.jump.total_frames);
            }
            
        }

        break;

    // case SDL_MOUSEMOTION:
    //     // Move label on mouse position
    //     // By the way, that's how you can implement a custom cursor
    //     ctx.welcome_text.sprite.transform.x = event->motion.x;
    //     ctx.welcome_text.sprite.transform.y = event->motion.y;
    //     break;
    }
}

static void update_and_render_scene(float delta)
{
    // Handling "continuous" events, which are now repeatable
    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT]) ctx.player.transform.x -= 640* delta;
    if (keys[SDL_SCANCODE_RIGHT]) ctx.player.transform.x += 640* delta;

    //Update the explosion's frame once every 100ms
    // if (ng_interval_is_ready(&ctx.game_tick))
    // {
    //     ng_animated_set_frame(&ctx.jump, (ctx.jump.frame + 1)
    //             % ctx.jump.total_frames);
    // }

    ng_sprite_render(&ctx.player, ctx.game.renderer);
    ng_sprite_render(&ctx.jump.sprite, ctx.game.renderer);
    ng_sprite_render(&ctx.welcome_text.sprite, ctx.game.renderer);
}

int main()
{
    create_actors();
    ng_game_start_loop(&ctx.game,
            handle_event, update_and_render_scene);
}