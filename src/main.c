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
#define MAX_SNOWMEN 5

typedef enum {
    SCENE_START,
    SCENE_PLAYING,
    SCENE_GAME_OVER,
    SCENE_DEATH
} Scene;

typedef enum {
    DIRECTION_RIGHT,
    DIRECTION_LEFT
} Direction;

Direction cat_direction = DIRECTION_RIGHT;


static struct
{
    ng_game_t game;
    ng_interval_t game_tick, ghost_tick, mouse_tick, snowman_tick, sleep_tick;

    // A collection of assets used by entities
    // Ideally, they should have been automatically loaded
    // by iterating over the res/ folder and filling in a hastable
    SDL_Texture *run_texture, *jump_texture, *idle_texture, *attack_texture, *sleep_texture, *ghost_texture, *mouse_texture, *snowman_texture, *heart_texture, *background_texture;

    ng_animated_sprite_t run, jump, idle, attack, sleep, ghost, mouse, snowman[MAX_SNOWMEN];

    ng_sprite_t heart[4];

    Mix_Music *SB_bm;
    Mix_Chunk *run_sfx, *hurt_sfx, *attack_sfx, *purr_sfx;

    TTF_Font *main_font, *death_font, *win_font;
    ng_label_t start_text, death_text, win_text;

    Scene current_scene;

    bool is_jumping;
    bool is_running;
    bool is_attacking;
    bool run_sfx_playing;
    int run_sfx_channel;
    bool mau;

    float jump_velocity;  
    float gravity;        

    int ghost_count;
    int active_snowmen;
    int health;

} ctx;


bool check_collision(ng_animated_sprite_t *a, ng_animated_sprite_t *b) {

    SDL_FRect rect_a = {
        .x = a->sprite.transform.x,
        .y = a->sprite.transform.y,
        .w = a->sprite.transform.w,
        .h = a->sprite.transform.h,
    };

    SDL_FRect rect_b = {
        .x = b->sprite.transform.x,
        .y = b->sprite.transform.y + 30,
        .w = b->sprite.transform.w,
        .h = b->sprite.transform.h - 30,
    };

    return SDL_HasIntersectionF(&rect_a, &rect_b);
}

void render_cat(ng_sprite_t *sprite, SDL_Renderer *renderer, Direction direction) {
    SDL_RendererFlip flip = (direction == DIRECTION_LEFT) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    SDL_RenderCopyExF(renderer, sprite->texture, &sprite->src, &sprite->transform, 0, NULL, flip);

}

void reset_game_state() {
    ctx.health = 3;
    ctx.ghost_count = 0;
    ctx.is_attacking = false;
    ctx.is_jumping = false;
    ctx.is_running = false;

    ng_animated_set_frame(&ctx.idle, 0);
    ng_animated_set_frame(&ctx.run, 0);
    ng_animated_set_frame(&ctx.jump, 0);
    ng_animated_set_frame(&ctx.attack, 0);

    ctx.jump.sprite.transform.x = 100.0f;
    ctx.jump.sprite.transform.y = FLOOR;
    ctx.run.sprite.transform.x = 100.0f;
    ctx.run.sprite.transform.y = FLOOR;
    ctx.attack.sprite.transform.x = 100.0f;
    ctx.attack.sprite.transform.y = FLOOR;
    ctx.idle.sprite.transform.x = 100.0f;
    ctx.idle.sprite.transform.y = FLOOR;

    ctx.ghost.sprite.transform.x = ng_random_int_in_range(0, WIDTH - 64);
    ctx.ghost.sprite.transform.y = -64;

    ctx.active_snowmen = 0;
    for (int i = 0; i < MAX_SNOWMEN; i++) {
        ctx.snowman[i].sprite.transform.x = ng_random_int_in_range(0, WIDTH - 64);
        ctx.snowman[i].sprite.transform.y = -64;
    }

    ctx.mau = true;
    ctx.mouse.sprite.transform.x = -64;

    ctx.current_scene = SCENE_START;
}



static void create_actors(void)
{
    ng_game_create(&ctx.game, "Cat", WIDTH, HEIGHT); //creates window

    ctx.main_font = TTF_OpenFont("assets/free_mono.ttf", 20);
    ctx.death_font = TTF_OpenFont("assets/free_mono.ttf", 32);
    ctx.win_font = TTF_OpenFont("assets/free_mono.ttf", 32);

 
    //load textures
    ctx.run_texture = IMG_LoadTexture(ctx.game.renderer, "assets/cat/run.png");
    ctx.jump_texture = IMG_LoadTexture(ctx.game.renderer, "assets/cat/jump.png");
    ctx.idle_texture = IMG_LoadTexture(ctx.game.renderer, "assets/cat/idle.png");
    ctx.attack_texture = IMG_LoadTexture(ctx.game.renderer, "assets/cat/attack.png");
    ctx.sleep_texture = IMG_LoadTexture(ctx.game.renderer, "assets/cat/sleep.png");
    ctx.ghost_texture = IMG_LoadTexture(ctx.game.renderer, "assets/characters/ghost.png");
    ctx.mouse_texture = IMG_LoadTexture(ctx.game.renderer, "assets/characters/mouse.png");
    ctx.snowman_texture = IMG_LoadTexture(ctx.game.renderer, "assets/characters/snowman.png");
    ctx.heart_texture = IMG_LoadTexture(ctx.game.renderer, "assets/heart.png");
    ctx.background_texture = IMG_LoadTexture(ctx.game.renderer, "assets/bg.png");

    //timers
    ng_interval_create(&ctx.game_tick, 60);
    ng_interval_create(&ctx.ghost_tick, 150);
    ng_interval_create(&ctx.snowman_tick, 2000);
    ng_interval_create(&ctx.sleep_tick, 300);
    
    //create animations
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

    ng_animated_create(&ctx.attack, ctx.attack_texture, 9);  //attack
    ng_sprite_set_scale(&ctx.attack.sprite, 2.0f);
    ctx.attack.sprite.transform.x = 100.0f;
    ctx.attack.sprite.transform.y = FLOOR;
    
    ng_animated_create(&ctx.sleep, ctx.sleep_texture, 3);  //sleep
    ng_sprite_set_scale(&ctx.sleep.sprite, 4.0f);
    ctx.sleep.sprite.transform.x = (WIDTH - ctx.sleep.sprite.transform.w) / 2.0f;
    ctx.sleep.sprite.transform.y = (HEIGHT - ctx.sleep.sprite.transform.h) / 2.0f;

    ng_animated_create(&ctx.ghost, ctx.ghost_texture, 2);  //ghost
    ng_sprite_set_scale(&ctx.ghost.sprite, 4.0f);
    ctx.ghost.sprite.transform.x = ng_random_int_in_range(0, WIDTH - 64);
    ctx.ghost.sprite.transform.y = -64;

    ng_animated_create(&ctx.mouse, ctx.mouse_texture, 4);  //mouse
    ng_sprite_set_scale(&ctx.mouse.sprite, 2.0f);    
    ctx.mouse.sprite.transform.x = -64.0f;
    ctx.mouse.sprite.transform.y = FLOOR;

    for (int i = 0; i < MAX_SNOWMEN; i++) {
        ng_animated_create(&ctx.snowman[i], ctx.snowman_texture, 5);  //snowman
        ng_sprite_set_scale(&ctx.snowman[i].sprite, 5.0f);
        ctx.snowman[i].sprite.transform.x = ng_random_int_in_range(0, WIDTH - 64);
        ctx.snowman[i].sprite.transform.y = -64;
    }

    for (int i = 0; i < 4; i++) {
        ng_sprite_create(&ctx.heart[i], ctx.heart_texture);
        ng_sprite_set_scale(&ctx.heart[i], 1.0f);
        ctx.heart[i].transform.x = i * 40;
    }
    
    //load audio
    ctx.SB_bm = ng_music_load("assets/audio/OST 1 - Silver Bells (Loopable).ogg");
    ctx.run_sfx = ng_audio_load("assets/audio/run.wav");
    ctx.hurt_sfx = ng_audio_load("assets/audio/hurt.wav");
    ctx.attack_sfx = ng_audio_load("assets/audio/attack.wav");
    ctx.purr_sfx = ng_audio_load("assets/audio/purr.wav");


    //load text
    ng_label_create(&ctx.start_text, ctx.main_font, 500);
    ng_label_set_content(&ctx.start_text, ctx.game.renderer,
            "Help the little cat save Christmas. "
            "Dodge the falling snowmen. \n"
            "Attack the ghosts with \"space\". \n"
            "press \"space\" to continue ");
    ctx.start_text.sprite.transform.x = (WIDTH - 400) / 2.0f;
    ctx.start_text.sprite.transform.y = (HEIGHT - 150) / 2.0f;

    ng_label_create(&ctx.death_text, ctx.death_font, 175);
    ng_label_set_content(&ctx.death_text, ctx.game.renderer,
            "Game Over"
            );
    ctx.death_text.sprite.transform.x = (WIDTH - 175) / 2.0f;
    ctx.death_text.sprite.transform.y = (HEIGHT - 125) / 2.0f;

    ng_label_create(&ctx.win_text, ctx.win_font, 285);
    ng_label_set_content(&ctx.win_text, ctx.game.renderer,
            "Congratulations\n"
            );
    ctx.win_text.sprite.transform.x = (WIDTH - 285) / 2.0f;
    ctx.win_text.sprite.transform.y = (HEIGHT - 300) / 2.0f;

#ifndef NO_AUDIO

    Mix_VolumeMusic(16);  //background music at lower volume
    Mix_VolumeChunk(ctx.run_sfx, 128);  //running sound at full volume
    Mix_VolumeChunk(ctx.hurt_sfx, 128);
    Mix_VolumeChunk(ctx.attack_sfx, 128);
    Mix_VolumeChunk(ctx.purr_sfx, 128);

#endif

    ctx.is_jumping = false; 
    ctx.is_running = false; 
    ctx.is_attacking = false;
    ctx.run_sfx_playing = false;
    ctx.mau = true;
    ctx.run_sfx_channel = -1;

    ctx.gravity = 1451.25f;      //pixels per second squared
    ctx.jump_velocity = 0.0f;  //initially not moving

    ctx.ghost_count = 0;
    ctx.health = 3;

    //start background music once looping it indefinitely
    ng_music_play(ctx.SB_bm);
}

static void handle_event(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:
        if (event->key.keysym.sym == SDLK_w || event->key.keysym.sym == SDLK_UP) {
            //only start the jump animation if it's not already jumping
            if (!ctx.is_jumping)
            {
                ctx.is_jumping = true;

                ctx.jump_velocity = -304.76f;  //initial upward velocity
                //reset the jump animation to the first frame
                ng_animated_set_frame(&ctx.jump, 0);
            }
        }

        

        if (event->key.keysym.sym == SDLK_SPACE) {
            //only start the attack animation if it's not already attacking
            if (!ctx.is_attacking)
            {
                ctx.is_attacking = true;
                //reset the attack animation to the first frame
                ng_animated_set_frame(&ctx.attack, 0);
            }
        }
        
        if (event->key.keysym.sym == SDLK_RIGHT || event->key.keysym.sym == SDLK_d) {
            //only start the run animation if it's not already running
            if (!ctx.is_running)
            {
                ctx.is_running = true;
                //reset the run animation to the first frame
                ng_animated_set_frame(&ctx.run, 0);
            }

            if (!ctx.run_sfx_playing) {
                ctx.run_sfx_channel = ng_return_channel(ctx.run_sfx, -1);
                ctx.run_sfx_playing = true;   //mark that the sound has started
            }
        }

        if (event->key.keysym.sym == SDLK_LEFT || event->key.keysym.sym == SDLK_a) {
            //only start the run animation if it's not already running
            if (!ctx.is_running)
            {
                ctx.is_running = true;
                //reset the run animation to the first frame
                ng_animated_set_frame(&ctx.run, 0);
            }

            if (!ctx.run_sfx_playing) {
                ctx.run_sfx_channel = ng_return_channel(ctx.run_sfx, -1);
                ctx.run_sfx_playing = true;   //mark that the sound has started
            }
        }
        break;

    case SDL_KEYUP:
        if (event->key.keysym.sym == SDLK_RIGHT || event->key.keysym.sym == SDLK_d) {
            //stop running animation when the right key is released
            ctx.is_running = false;
            if (ctx.run_sfx_playing) {
                //stop the running sound when the key is released
                #ifndef NO_AUDIO    
                    Mix_HaltChannel(ctx.run_sfx_channel);  //halt the specific channel
                #endif
                ctx.run_sfx_playing = false;  //mark that the sound has stopped
            }            
        }

        if (event->key.keysym.sym == SDLK_LEFT || event->key.keysym.sym == SDLK_a) {
            //stop running animation when the right key is released
            ctx.is_running = false;
            if (ctx.run_sfx_playing) {
                //stop the running sound when the key is released
                #ifndef NO_AUDIO    
                    Mix_HaltChannel(ctx.run_sfx_channel);  //halt the specific channel
                #endif
                ctx.run_sfx_playing = false;  //mark that the sound has stopped
            }        
        }

    }
}


static void update_and_render_scene(float delta)
{

    SDL_RenderCopy(ctx.game.renderer, ctx.background_texture, NULL, NULL);

    // Handling "continuous" events, which are now repeatable
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]){ //move left
        if (ctx.run.sprite.transform.x > 0) { //wall boundary
            cat_direction = DIRECTION_LEFT;
            ctx.run.sprite.transform.x -= SPEED * delta; 
            ctx.idle.sprite.transform.x -= SPEED * delta;
            ctx.jump.sprite.transform.x -= SPEED * delta;
            ctx.attack.sprite.transform.x -= SPEED * delta;  
        }
            
    } 

    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]){ //move right
        if (ctx.run.sprite.transform.x < WIDTH - 64){ //wall boundary
            cat_direction = DIRECTION_RIGHT;
            ctx.run.sprite.transform.x += SPEED* delta;
            ctx.jump.sprite.transform.x += SPEED* delta;
            ctx.idle.sprite.transform.x += SPEED* delta;
            ctx.attack.sprite.transform.x += SPEED* delta;
        }
    }


    if (ctx.ghost.sprite.transform.y < FLOOR) {
        ctx.ghost.sprite.transform.y += 100 * delta;  //make the ghost fall
    }

    for (int i = 0; i < ctx.active_snowmen; i++) {
        if (ctx.snowman[i].sprite.transform.y < HEIGHT - 30) {
            ctx.snowman[i].sprite.transform.y += 100 * delta;  //make the snowman fall
        }else {
            //once the snowman reaches the ground, reset its position to top
            ctx.snowman[i].sprite.transform.y = -64;  
            ctx.snowman[i].sprite.transform.x = ng_random_int_in_range(0, WIDTH - 64);  //random horizontal position
        }
    }

    //reset snowman to the top when it collides with the cat
    for (int i = 0; i < ctx.active_snowmen; i++) {
        if (check_collision(&ctx.snowman[i], &ctx.run)) {
            ng_audio_play(ctx.hurt_sfx);
            ctx.health--;
            ctx.snowman[i].sprite.transform.y = -64; 
            ctx.snowman[i].sprite.transform.x = ng_random_int_in_range(0, WIDTH - 64);
        }
    }

    //reset ghost to the top when cat attacks and collides with the ghost
    if (ctx.is_attacking) {
            if (check_collision(&ctx.ghost, &ctx.attack)) {
                ctx.ghost_count++;
                ng_audio_play(ctx.attack_sfx);
                ctx.ghost.sprite.transform.y = -64;  
                ctx.ghost.sprite.transform.x = ng_random_int_in_range(0, WIDTH - 64);
            }
            if (check_collision(&ctx.mouse, &ctx.attack)) {
                ng_audio_play(ctx.attack_sfx);
                ctx.mouse.sprite.transform.x = -64;
                ctx.health++;
                ctx.mau = false;
            }
    }

    //add more snowmen
    if (ng_interval_is_ready(&ctx.snowman_tick) && ctx.active_snowmen < MAX_SNOWMEN) {
        ctx.active_snowmen++;
    }

    //cat animations
    if(ctx.is_attacking){
        //move to the next frame of the attacking animation
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.attack, (ctx.attack.frame + 1) % ctx.attack.total_frames);
        }

        if (ctx.attack.frame == ctx.attack.total_frames - 1) {
            ctx.is_attacking = false;  //animation has finished stop attacking

        }

    }else if (ctx.is_jumping) {
        //only apply vertical motion during active jump frames 3 to 10
    if (ctx.jump.frame >= 3 && ctx.jump.frame <= 10) {
        ctx.jump_velocity += ctx.gravity * delta;  
        ctx.jump.sprite.transform.y += ctx.jump_velocity * delta; 

        //check if the sprite lands early due to gravity
        if (ctx.jump.sprite.transform.y >= FLOOR) {
            ctx.jump.sprite.transform.y = FLOOR;  //snap to ground
            ctx.is_jumping = false;               //end jump
            ctx.jump_velocity = 0.0f;             //reset velocity
        }

            //transition to running if the right key is held
            if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
                ctx.is_running = true;
                ng_animated_set_frame(&ctx.run, 0);  //reset running animation
            }
            //transition to running if the left key is held
            if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
                ctx.is_running = true;
                ng_animated_set_frame(&ctx.run, 0);  //reset running animation
            }
        }

        //move to the next frame of the jump animation
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.jump, (ctx.jump.frame + 1) % ctx.jump.total_frames);
        }
    }else if(ctx.is_running){
        //move to the next frame of the running animation
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.run, (ctx.run.frame + 1) % ctx.run.total_frames);
        }
    }else{
        if (ng_interval_is_ready(&ctx.game_tick)) {
            ng_animated_set_frame(&ctx.idle, (ctx.idle.frame + 1) % ctx.idle.total_frames);
        }
    }

    //ghost animation
    if (ng_interval_is_ready(&ctx.ghost_tick)) {
        ng_animated_set_frame(&ctx.ghost, (ctx.ghost.frame + 1) % ctx.ghost.total_frames);
    }



    // Render animations
    if (ctx.is_attacking){
        render_cat(&ctx.attack.sprite, ctx.game.renderer, cat_direction);
    }else if (ctx.is_jumping) {
        render_cat(&ctx.jump.sprite, ctx.game.renderer, cat_direction);
    } else if (ctx.is_running){
        render_cat(&ctx.run.sprite, ctx.game.renderer, cat_direction);
    }else {
        render_cat(&ctx.idle.sprite, ctx.game.renderer, cat_direction); //show idle when doing no actions
    }

    ng_sprite_render(&ctx.ghost.sprite, ctx.game.renderer);

    for (int i = 0; i < ctx.active_snowmen; i++) {
        ng_sprite_render(&ctx.snowman[i].sprite, ctx.game.renderer);
    }
    //SDL_Delay(2000);
    if (ctx.mau){
        ng_sprite_render(&ctx.mouse.sprite, ctx.game.renderer);
        ctx.mouse.sprite.transform.x += 50* delta;
    }

    if (ctx.health >= 4){
        ctx.health = 4;
        for (int i = 0; i < 4; i++) {
            ng_sprite_render(&ctx.heart[i], ctx.game.renderer);
        }
    }else if(ctx.health == 3) {
       for (int i = 0; i < 3; i++) {
            ng_sprite_render(&ctx.heart[i], ctx.game.renderer);
        }
    }else if(ctx.health == 2) {
        for (int i = 0; i < 2; i++) {
            ng_sprite_render(&ctx.heart[i], ctx.game.renderer);
        }
    }else if(ctx.health == 1) {
        ng_sprite_render(&ctx.heart[0], ctx.game.renderer);
    }else if(ctx.health == 0) {
       ctx.current_scene = SCENE_DEATH; 
    }
    if(ctx.ghost_count == 10) {
        ctx.current_scene = SCENE_GAME_OVER;
    }
}

static void game_loop(float delta) {
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    switch (ctx.current_scene) {
        case SCENE_START:
            ng_sprite_render(&ctx.start_text.sprite, ctx.game.renderer);

            if (keys[SDL_SCANCODE_SPACE]){
                ctx.current_scene = SCENE_PLAYING;
            }
            break;
        case SCENE_PLAYING:
            update_and_render_scene(delta);
            break;
        case SCENE_GAME_OVER:
            ng_sprite_render(&ctx.win_text.sprite, ctx.game.renderer);

            if (ng_interval_is_ready(&ctx.sleep_tick)) {
                ng_animated_set_frame(&ctx.sleep, (ctx.sleep.frame + 1) % ctx.sleep.total_frames);
            }

            ng_sprite_render(&ctx.sleep.sprite, ctx.game.renderer);

            ng_audio_play(ctx.purr_sfx);

            break;
        case SCENE_DEATH:
            ng_sprite_render(&ctx.death_text.sprite, ctx.game.renderer);

            if (keys[SDL_SCANCODE_SPACE]){
                reset_game_state();
            }
            break;
    }
}

int main()
{
    create_actors();
    ctx.current_scene = SCENE_START;
    
    ng_game_start_loop(&ctx.game,
            handle_event, game_loop);
}