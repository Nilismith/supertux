/*
  setup.c
  
  Super Tux - Setup
  
  by Bill Kendrick
  bill@newbreedsoftware.com
  http://www.newbreedsoftware.com/supertux/
  
  April 11, 2000 - March 15, 2004
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <SDL.h>
#include <SDL_image.h>
#ifndef NOOPENGL
#include <SDL_opengl.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#ifndef WIN32
#include <libgen.h>
#endif
#include <ctype.h>

#include "defines.h"
#include "globals.h"
#include "setup.h"
#include "screen.h"
#include "texture.h"
#include "menu.h"
#include "gameloop.h"
#include "configfile.h"
#include "scene.h"

#ifdef WIN32
#define mkdir(dir, mode)    mkdir(dir)
// on win32 we typically don't want LFS paths
#undef DATA_PREFIX
#define DATA_PREFIX "./data/"
#endif

/* Local function prototypes: */

void seticon(void);
void usage(char * prog, int ret);

/* Does the given file exist and is it accessible? */
int faccessible(const char *filename)
{
  struct stat filestat;
  if (stat(filename, &filestat) == -1)
    {
      return false;
    }
  else
    {
      if(S_ISREG(filestat.st_mode))
        return true;
      else
        return false;
    }
}

/* Can we write to this location? */
int fwriteable(const char *filename)
{
  FILE* fi;
  fi = fopen(filename, "wa");
  if (fi == NULL)
    {
      return false;
    }
  return true;
}

/* Makes sure a directory is created in either the SuperTux home directory or the SuperTux base directory.*/
int fcreatedir(const char* relative_dir)
{
  char path[1024];
  snprintf(path, 1024, "%s/%s/", st_dir, relative_dir);
  if(mkdir(path,0755) != 0)
    {
      snprintf(path, 1024, "%s/%s/", datadir.c_str(), relative_dir);
      if(mkdir(path,0755) != 0)
        {
          return false;
        }
      else
        {
          return true;
        }
    }
  else
    {
      return true;
    }
}

FILE * opendata(const char * rel_filename, const char * mode)
{
  char * filename = NULL;
  FILE * fi;

  filename = (char *) malloc(sizeof(char) * (strlen(st_dir) +
                                             strlen(rel_filename) + 1));

  strcpy(filename, st_dir);
  /* Open the high score file: */

  strcat(filename, rel_filename);

  /* Try opening the file: */
  fi = fopen(filename, mode);

  if (fi == NULL)
    {
      fprintf(stderr, "Warning: Unable to open the file \"%s\" ", filename);

      if (strcmp(mode, "r") == 0)
        fprintf(stderr, "for read!!!\n");
      else if (strcmp(mode, "w") == 0)
        fprintf(stderr, "for write!!!\n");
    }
  free( filename );

  return(fi);
}

/* Get all names of sub-directories in a certain directory. */
/* Returns the number of sub-directories found. */
/* Note: The user has to free the allocated space. */
string_list_type dsubdirs(const char *rel_path,const  char* expected_file)
{
  DIR *dirStructP;
  struct dirent *direntp;
  string_list_type sdirs;
  char filename[1024];
  char path[1024];

  string_list_init(&sdirs);
  sprintf(path,"%s/%s",st_dir,rel_path);
  if((dirStructP = opendir(path)) != NULL)
    {
      while((direntp = readdir(dirStructP)) != NULL)
        {
          char absolute_filename[1024];
          struct stat buf;

          sprintf(absolute_filename, "%s/%s", path, direntp->d_name);

          if (stat(absolute_filename, &buf) == 0 && S_ISDIR(buf.st_mode))
            {
              if(expected_file != NULL)
                {
                  sprintf(filename,"%s/%s/%s",path,direntp->d_name,expected_file);
                  if(!faccessible(filename))
                    continue;
                }

              string_list_add_item(&sdirs,direntp->d_name);
            }
        }
      closedir(dirStructP);
    }

  sprintf(path,"%s/%s",datadir.c_str(),rel_path);
  if((dirStructP = opendir(path)) != NULL)
    {
      while((direntp = readdir(dirStructP)) != NULL)
        {
          char absolute_filename[1024];
          struct stat buf;

          sprintf(absolute_filename, "%s/%s", path, direntp->d_name);

          if (stat(absolute_filename, &buf) == 0 && S_ISDIR(buf.st_mode))
            {
              if(expected_file != NULL)
                {
                  sprintf(filename,"%s/%s/%s",path,direntp->d_name,expected_file);
                  if(!faccessible(filename))
                    {
                      continue;
                    }
                  else
                    {
                      sprintf(filename,"%s/%s/%s/%s",st_dir,rel_path,direntp->d_name,expected_file);
                      if(faccessible(filename))
                        continue;
                    }
                }

              string_list_add_item(&sdirs,direntp->d_name);
            }
        }
      closedir(dirStructP);
    }

  return sdirs;
}

string_list_type dfiles(const char *rel_path, const  char* glob, const  char* exception_str)
{
  DIR *dirStructP;
  struct dirent *direntp;
  string_list_type sdirs;
  char path[1024];

  string_list_init(&sdirs);
  sprintf(path,"%s/%s",st_dir,rel_path);
  if((dirStructP = opendir(path)) != NULL)
    {
      while((direntp = readdir(dirStructP)) != NULL)
        {
          char absolute_filename[1024];
          struct stat buf;

          sprintf(absolute_filename, "%s/%s", path, direntp->d_name);

          if (stat(absolute_filename, &buf) == 0 && S_ISREG(buf.st_mode))
            {
              if(exception_str != NULL)
                {
                  if(strstr(direntp->d_name,exception_str) != NULL)
                    continue;
                }
              if(glob != NULL)
                if(strstr(direntp->d_name,glob) == NULL)
                  continue;

              string_list_add_item(&sdirs,direntp->d_name);
            }
        }
      closedir(dirStructP);
    }

  sprintf(path,"%s/%s",datadir.c_str(),rel_path);
  if((dirStructP = opendir(path)) != NULL)
    {
      while((direntp = readdir(dirStructP)) != NULL)
        {
          char absolute_filename[1024];
          struct stat buf;

          sprintf(absolute_filename, "%s/%s", path, direntp->d_name);

          if (stat(absolute_filename, &buf) == 0 && S_ISREG(buf.st_mode))
            {
              if(exception_str != NULL)
                {
                  if(strstr(direntp->d_name,exception_str) != NULL)
                    continue;
                }
              if(glob != NULL)
                if(strstr(direntp->d_name,glob) == NULL)
                  continue;

              string_list_add_item(&sdirs,direntp->d_name);
            }
        }
      closedir(dirStructP);
    }

  return sdirs;
}

void free_strings(char **strings, int num)
{
  int i;
  for(i=0; i < num; ++i)
    free(strings[i]);
}

/* --- SETUP --- */
/* Set SuperTux configuration and save directories */
void st_directory_setup(void)
{
  char *home;
  char str[1024];
  /* Get home directory (from $HOME variable)... if we can't determine it,
     use the current directory ("."): */
  if (getenv("HOME") != NULL)
    home = getenv("HOME");
  else
    home = ".";

  st_dir = (char *) malloc(sizeof(char) * (strlen(home) +
                                           strlen("/.supertux") + 1));
  strcpy(st_dir, home);
  strcat(st_dir, "/.supertux");

  /* Remove .supertux config-file from old SuperTux versions */
  if(faccessible(st_dir))
    {
      remove
        (st_dir);
    }

  st_save_dir = (char *) malloc(sizeof(char) * (strlen(st_dir) + strlen("/save") + 1));

  strcpy(st_save_dir,st_dir);
  strcat(st_save_dir,"/save");

  /* Create them. In the case they exist they won't destroy anything. */
  mkdir(st_dir, 0755);
  mkdir(st_save_dir, 0755);

  sprintf(str, "%s/levels", st_dir);
  mkdir(str, 0755);

  // User has not that a datadir, so we try some magic
  if (datadir.empty())
    {
#ifndef WIN32
      // Detect datadir
      char exe_file[PATH_MAX];
      if (readlink("/proc/self/exe", exe_file, PATH_MAX) < 0)
        {
          puts("Couldn't read /proc/self/exe, using default path: " DATA_PREFIX);
          datadir = DATA_PREFIX;
        }
      else
        {
          std::string exedir = std::string(dirname(exe_file)) + "/";
          
          datadir = exedir + "../data/"; // SuperTux run from source dir
          if (access(datadir.c_str(), F_OK) != 0)
            {
              datadir = exedir + "../share/supertux/"; // SuperTux run from PATH
              if (access(datadir.c_str(), F_OK) != 0) 
                { // If all fails, fall back to compiled path
                  datadir = DATA_PREFIX; 
                }
            }
        }
#else
  datadir = DATA_PREFIX;
#endif
    }
  printf("Datadir: %s\n", datadir.c_str());
}

/* Create and setup menus. */
void st_menu(void)
{
  main_menu      = new Menu();
  options_menu   = new Menu();
  options_controls_menu   = new Menu();
  load_game_menu = new Menu();
  save_game_menu = new Menu();
  game_menu      = new Menu();
  highscore_menu = new Menu();

  main_menu->additem(MN_LABEL,"Main Menu",0,0);
  main_menu->additem(MN_HL,"",0,0);
  main_menu->additem(MN_ACTION,"Start Game",0,0);
  main_menu->additem(MN_GOTO,"Load Game",0,load_game_menu);
  main_menu->additem(MN_GOTO,"Options",0,options_menu);
  main_menu->additem(MN_ACTION,"Level editor",0,0);
  main_menu->additem(MN_ACTION,"Credits",0,0);
  main_menu->additem(MN_HL,"",0,0);
  main_menu->additem(MN_ACTION,"Quit",0,0);

  options_menu->additem(MN_LABEL,"Options",0,0);
  options_menu->additem(MN_HL,"",0,0);
  options_menu->additem(MN_TOGGLE,"Fullscreen",use_fullscreen,0);
  if(audio_device)
    {
      options_menu->additem(MN_TOGGLE,"Sound     ",use_sound,0);
      options_menu->additem(MN_TOGGLE,"Music     ",use_music,0);
    }
  else
    {
      options_menu->additem(MN_DEACTIVE,"Sound     ",use_sound,0);
      options_menu->additem(MN_DEACTIVE,"Music     ",use_music,0);
    }
  options_menu->additem(MN_TOGGLE,"Show FPS  ",show_fps,0);
  options_menu->additem(MN_GOTO,"Controls  ",0,options_controls_menu);
  options_menu->additem(MN_HL,"",0,0);
  options_menu->additem(MN_BACK,"Back",0,0);
  
  options_controls_menu->additem(MN_LABEL,"Controls",0,0);
  options_controls_menu->additem(MN_HL,"",0,0);
  options_controls_menu->additem(MN_CONTROLFIELD,"Move Right",tux.keymap.right,0);
  options_controls_menu->additem(MN_HL,"",0,0);
  options_controls_menu->additem(MN_BACK,"Back",0,0);

  load_game_menu->additem(MN_LABEL,"Load Game",0,0);
  load_game_menu->additem(MN_HL,"",0,0);
  load_game_menu->additem(MN_DEACTIVE,"Slot 1",0,0);
  load_game_menu->additem(MN_DEACTIVE,"Slot 2",0,0);
  load_game_menu->additem(MN_DEACTIVE,"Slot 3",0,0);
  load_game_menu->additem(MN_DEACTIVE,"Slot 4",0,0);
  load_game_menu->additem(MN_DEACTIVE,"Slot 5",0,0);
  load_game_menu->additem(MN_HL,"",0,0);
  load_game_menu->additem(MN_BACK,"Back",0,0);

  save_game_menu->additem(MN_LABEL,"Save Game",0,0);
  save_game_menu->additem(MN_HL,"",0,0);
  save_game_menu->additem(MN_DEACTIVE,"Slot 1",0,0);
  save_game_menu->additem(MN_DEACTIVE,"Slot 2",0,0);
  save_game_menu->additem(MN_DEACTIVE,"Slot 3",0,0);
  save_game_menu->additem(MN_DEACTIVE,"Slot 4",0,0);
  save_game_menu->additem(MN_DEACTIVE,"Slot 5",0,0);
  save_game_menu->additem(MN_HL,"",0,0);
  save_game_menu->additem(MN_BACK,"Back",0,0);

  game_menu->additem(MN_LABEL,"InGame Menu",0,0);
  game_menu->additem(MN_HL,"",0,0);
  game_menu->additem(MN_ACTION,"Return To Game",0,0);
  game_menu->additem(MN_GOTO,"Save Game",0,save_game_menu);
  game_menu->additem(MN_GOTO,"Load Game",0,load_game_menu);
  game_menu->additem(MN_GOTO,"Options",0,options_menu);
  game_menu->additem(MN_HL,"",0,0);
  game_menu->additem(MN_ACTION,"Quit Game",0,0);

  highscore_menu->additem(MN_TEXTFIELD,"Enter your name:",0,0);
}

void update_load_save_game_menu(Menu* pmenu, int load)
{
  for(int i = 2; i < 7; ++i)
    {
      char *tmp;
      slotinfo(&tmp,i-1);
      if(load && strlen(tmp) == strlen("Slot X - Free") )
        pmenu->item[i].kind = MN_DEACTIVE;
      else
        pmenu->item[i].kind = MN_ACTION;
      menu_item_change_text(&pmenu->item[i],tmp);
      free(tmp);
    }
}

void process_save_load_game_menu(int save)
{
  int slot;
  switch (slot = (save ? save_game_menu->check() : load_game_menu->check()))
    {
    default:
      if(slot != -1)
        {
          if(save)
            {
              savegame(slot - 1);
            }
          else
            {
              if (game_started)
                {
                  gameloop("default",slot - 1,ST_GL_LOAD_GAME);
                  show_menu = true;
                  Menu::set_current(main_menu);
                }
              else
                loadgame(slot - 1);
            }
          st_pause_ticks_stop();
        }
      break;
    }
}

/* Handle changes made to global settings in the options menu. */
void process_options_menu(void)
{
  switch (options_menu->check())
    {
    case 2:
      if(use_fullscreen != options_menu->item[2].toggled)
        {
          use_fullscreen = !use_fullscreen;
          st_video_setup();
        }
      break;
    case 3:
      if(use_sound != options_menu->item[3].toggled)
        use_sound = !use_sound;
      break;
    case 4:
      if(use_music != options_menu->item[4].toggled)
        {
          if(use_music)
            {
              if(playing_music())
                {
                  halt_music();
                }
              use_music = false;
            }
          else
            {
              use_music = true;
              if (!playing_music())
                {
                  play_current_music();
                }
            }
        }
      break;
    case 5:
      if(show_fps != options_menu->item[5].toggled)
        show_fps = !show_fps;
      break;
    }
}

void st_general_setup(void)
{
  /* Seed random number generator: */

  srand(SDL_GetTicks());

  /* Set icon image: */

  seticon();

  /* Unicode needed for input handling: */

  SDL_EnableUNICODE(1);

  /* Load global images: */

  text_load(&black_text, datadir + "/images/status/letters-black.png", TEXT_TEXT, 16,18);
  text_load(&gold_text,datadir + "/images/status/letters-gold.png", TEXT_TEXT, 16,18);
  text_load(&blue_text,datadir + "/images/status/letters-blue.png", TEXT_TEXT, 16,18);
  text_load(&red_text,datadir + "/images/status/letters-red.png", TEXT_TEXT, 16,18);
  text_load(&white_text,datadir + "/images/status/letters-white.png", TEXT_TEXT, 16,18);
  text_load(&white_small_text,datadir + "/images/status/letters-white-small.png", TEXT_TEXT, 8,9);
  text_load(&white_big_text,datadir + "/images/status/letters-white-big.png", TEXT_TEXT, 20,23);
  text_load(&yellow_nums,datadir + "/images/status/numbers.png", TEXT_NUM, 32,32);

  /* Load GUI/menu images: */
  texture_load(&checkbox, datadir + "/images/status/checkbox.png", USE_ALPHA);
  texture_load(&checkbox_checked, datadir + "/images/status/checkbox-checked.png", USE_ALPHA);
  texture_load(&back, datadir + "/images/status/back.png", USE_ALPHA);
  texture_load(&arrow_left, datadir + "/images/icons/left.png", USE_ALPHA);
  texture_load(&arrow_right, datadir + "/images/icons/right.png", USE_ALPHA);

  /* Load the mouse-cursor */
  mouse_cursor = new MouseCursor( datadir + "/images/status/mousecursor.png",1);
  
}

void st_general_free(void)
{

  /* Free global images: */

  text_free(&black_text);
  text_free(&gold_text);
  text_free(&white_text);
  text_free(&blue_text);
  text_free(&red_text);
  text_free(&white_small_text);
  text_free(&white_big_text);

  /* Free GUI/menu images: */
  texture_free(&checkbox);
  texture_free(&checkbox_checked);
  texture_free(&back);
  texture_free(&arrow_left);
  texture_free(&arrow_right);

  /* Free mouse-cursor */
  delete mouse_cursor;
  
  /* Free menus */
  delete main_menu;
  delete game_menu;
  delete options_menu;
  delete highscore_menu;
  delete save_game_menu;
  delete load_game_menu;
}

void st_video_setup(void)
{

  if(screen != NULL)
    SDL_FreeSurface(screen);

  /* Init SDL Video: */

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr,
              "\nError: I could not initialize video!\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());
      exit(1);
    }

  /* Open display: */

  if(use_gl)
    st_video_setup_gl();
  else
    st_video_setup_sdl();

  texture_setup();

  /* Set window manager stuff: */

  SDL_WM_SetCaption("Super Tux", "Super Tux");

}

void st_video_setup_sdl(void)
{
  SDL_FreeSurface(screen);

  if (use_fullscreen)
    {
      screen = SDL_SetVideoMode(640, 480, 0, SDL_FULLSCREEN ) ; /* | SDL_HWSURFACE); */
      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nWarning: I could not set up fullscreen video for "
                  "640x480 mode.\n"
                  "The Simple DirectMedia error that occured was:\n"
                  "%s\n\n", SDL_GetError());
          use_fullscreen = false;
        }
    }
  else
    {
      screen = SDL_SetVideoMode(640, 480, 0, SDL_HWSURFACE | SDL_DOUBLEBUF );

      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nError: I could not set up video for 640x480 mode.\n"
                  "The Simple DirectMedia error that occured was:\n"
                  "%s\n\n", SDL_GetError());
          exit(1);
        }
    }
}

void st_video_setup_gl(void)
{
#ifndef NOOPENGL

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  if (use_fullscreen)
    {
      screen = SDL_SetVideoMode(640, 480, 0, SDL_FULLSCREEN | SDL_OPENGL) ; /* | SDL_HWSURFACE); */
      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nWarning: I could not set up fullscreen video for "
                  "640x480 mode.\n"
                  "The Simple DirectMedia error that occured was:\n"
                  "%s\n\n", SDL_GetError());
          use_fullscreen = false;
        }
    }
  else
    {
      screen = SDL_SetVideoMode(640, 480, 0, SDL_OPENGL);

      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nError: I could not set up video for 640x480 mode.\n"
                  "The Simple DirectMedia error that occured was:\n"
                  "%s\n\n", SDL_GetError());
          exit(1);
        }
    }

  /*
   * Set up OpenGL for 2D rendering.
   */
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glViewport(0, 0, screen->w, screen->h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, screen->w, screen->h, 0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, 0.0f);

#endif

}

void st_joystick_setup(void)
{

  /* Init Joystick: */

  use_joystick = true;

  if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
    {
      fprintf(stderr, "Warning: I could not initialize joystick!\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());

      use_joystick = false;
    }
  else
    {
      /* Open joystick: */
      if (SDL_NumJoysticks() <= 0)
        {
          fprintf(stderr, "Warning: No joysticks are available.\n");

          use_joystick = false;
        }
      else
        {
          js = SDL_JoystickOpen(joystick_num);

          if (js == NULL)
            {
              fprintf(stderr, "Warning: Could not open joystick %d.\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", joystick_num, SDL_GetError());

              use_joystick = false;
            }
          else
            {
              /* Check for proper joystick configuration: */

              if (SDL_JoystickNumAxes(js) < 2)
                {
                  fprintf(stderr,
                          "Warning: Joystick does not have enough axes!\n");

                  use_joystick = false;
                }
              else
                {
                  if (SDL_JoystickNumButtons(js) < 2)
                    {
                      fprintf(stderr,
                              "Warning: "
                              "Joystick does not have enough buttons!\n");

                      use_joystick = false;
                    }
                }
            }
        }
    }
}

void st_audio_setup(void)
{

  /* Init SDL Audio silently even if --disable-sound : */

  if (audio_device)
    {
      if (SDL_Init(SDL_INIT_AUDIO) < 0)
        {
          /* only print out message if sound or music
             was not disabled at command-line
           */
          if (use_sound || use_music)
            {
              fprintf(stderr,
                      "\nWarning: I could not initialize audio!\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", SDL_GetError());
            }
          /* keep the programming logic the same :-)
             because in this case, use_sound & use_music' values are ignored
             when there's no available audio device
          */
          use_sound = false;
          use_music = false;
          audio_device = false;
        }
    }


  /* Open sound silently regarless the value of "use_sound": */

  if (audio_device)
    {
      if (open_audio(44100, AUDIO_S16, 2, 2048) < 0)
        {
          /* only print out message if sound or music
             was not disabled at command-line
           */
          if (use_sound || use_music)
            {
              fprintf(stderr,
                      "\nWarning: I could not set up audio for 44100 Hz "
                      "16-bit stereo.\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", SDL_GetError());
            }
          use_sound = false;
          use_music = false;
          audio_device = false;
        }
    }

}


/* --- SHUTDOWN --- */

void st_shutdown(void)
{
  close_audio();
  SDL_Quit();
  saveconfig();
}

/* --- ABORT! --- */

void st_abort(const std::string& reason, const std::string& details)
{
  fprintf(stderr, "\nError: %s\n%s\n\n", reason.c_str(), details.c_str());
  st_shutdown();
  abort();
}


/* Set Icon (private) */

void seticon(void)
{
  int masklen;
  Uint8 * mask;
  SDL_Surface * icon;


  /* Load icon into a surface: */

  icon = IMG_Load((datadir + "/images/icon.png").c_str());
  if (icon == NULL)
    {
      fprintf(stderr,
              "\nError: I could not load the icon image: %s%s\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", datadir.c_str(), "/images/icon.png", SDL_GetError());
      exit(1);
    }


  /* Create mask: */

  masklen = (((icon -> w) + 7) / 8) * (icon -> h);
  mask = (Uint8*) malloc(masklen * sizeof(Uint8));
  memset(mask, 0xFF, masklen);


  /* Set icon: */

  SDL_WM_SetIcon(icon, mask);


  /* Free icon surface & mask: */

  free(mask);
  SDL_FreeSurface(icon);
}


/* Parse command-line arguments: */

void parseargs(int argc, char * argv[])
{
  int i;

  loadconfig();

  /* Parse arguments: */

  for (i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "--fullscreen") == 0 ||
          strcmp(argv[i], "-f") == 0)
        {
          /* Use full screen: */

          use_fullscreen = true;
        }
      else if (strcmp(argv[i], "--joystick") == 0 || strcmp(argv[i], "-j") == 0)
        {
          assert(i+1 < argc);
          joystick_num = atoi(argv[++i]);
        }
      else if (strcmp(argv[i], "--worldmap") == 0)
        {
          launch_worldmap_mode = true;
        }
      else if (strcmp(argv[i], "--datadir") == 0 
               || strcmp(argv[i], "-d") == 0 )
        {
          assert(i+1 < argc);
          datadir = argv[++i];
        }
      else if (strcmp(argv[i], "--show-fps") == 0)
        {
          /* Use full screen: */

          show_fps = true;
        }
      else if (strcmp(argv[i], "--opengl") == 0 ||
               strcmp(argv[i], "-gl") == 0)
        {
#ifndef NOOPENGL
          /* Use OpengGL: */

          use_gl = true;
#endif

        }
      else if (strcmp(argv[i], "--usage") == 0)
        {
          /* Show usage: */

          usage(argv[0], 0);
        }
      else if (strcmp(argv[i], "--version") == 0)
        {
          /* Show version: */

          printf("Super Tux - version " VERSION "\n");
          exit(0);
        }
      else if (strcmp(argv[i], "--disable-sound") == 0)
        {
          /* Disable the compiled in sound feature */
          printf("Sounds disabled \n");
          use_sound = false;
        }
      else if (strcmp(argv[i], "--disable-music") == 0)
        {
          /* Disable the compiled in sound feature */
          printf("Music disabled \n");
          use_music = false;
        }
      else if (strcmp(argv[i], "--debug-mode") == 0)
        {
          /* Enable the debug-mode */
          debug_mode = true;

        }
      else if (strcmp(argv[i], "--help") == 0)
        {     /* Show help: */
          puts("Super Tux " VERSION "\n"
               "  Please see the file \"README.txt\" for more details.\n");
          printf("Usage: %s [OPTIONS] FILENAME\n\n", argv[0]);
          puts("Display Options:\n"
               "  --fullscreen        Run in fullscreen mode.\n"
               "  --opengl            If opengl support was compiled in, this will enable\n"
               "                      the EXPERIMENTAL OpenGL mode.\n"
               "\n"
               "Sound Options:\n"
               "  --disable-sound     If sound support was compiled in,  this will\n"
               "                      disable sound for this session of the game.\n"
               "  --disable-music     Like above, but this will disable music.\n"
               "\n"
               "Misc Options:\n"
               "  -j, --joystick NUM  Use joystick NUM (default: 0)\n" 
               "  --worldmap          Start in worldmap-mode (EXPERIMENTAL)\n"          
               "  -d, --datadir DIR   Load Game data from DIR (default: automatic)\n"
               "  --debug-mode        Enables the debug-mode, which is useful for developers.\n"
               "  --help              Display a help message summarizing command-line\n"
               "                      options, license and game controls.\n"
               "  --usage             Display a brief message summarizing command-line options.\n"
               "  --version           Display the version of SuperTux you're running.\n\n"
               );
          exit(0);
        }
      else if (argv[i][0] != '-')
        {
          level_startup_file = argv[i];
        }
      else
        {
          /* Unknown - complain! */

          usage(argv[0], 1);
        }
    }
}


/* Display usage: */

void usage(char * prog, int ret)
{
  FILE * fi;


  /* Determine which stream to write to: */

  if (ret == 0)
    fi = stdout;
  else
    fi = stderr;


  /* Display the usage message: */

  fprintf(fi, "Usage: %s [--fullscreen] [--opengl] [--disable-sound] [--disable-music] [--debug-mode] | [--usage | --help | --version] [--worldmap] FILENAME\n",
          prog);


  /* Quit! */

  exit(ret);
}

