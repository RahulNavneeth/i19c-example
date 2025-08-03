#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <limits.h>
#include "i19c.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"

typedef struct {
    int width;
    int height;
} Config;

typedef struct {
    const char **options;
    int optionCount;
    int selectedIndex;
    bool isOpen;
} DropdownState;

typedef struct {
    TTF_Font *font;
    SDL_Renderer *renderer;
} TextRenderData;

Config *get_config() {
    static Config config = { .width = 800, .height = 600 };
    return &config;
}

Clay_Dimensions MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    if (!text.chars || text.length < 0) {
        return (Clay_Dimensions){0.0f, 16.0f};
    }
    
    TextRenderData *data = (TextRenderData*)userData;
    if (!data || !data->font) {
        return (Clay_Dimensions){text.length * 8.0f, 20.0f};
    }
    
    char *tempStr = malloc(text.length + 1);
    memcpy(tempStr, text.chars, text.length);
    tempStr[text.length] = '\0';
    
    int w, h;
    TTF_GetStringSize(data->font, tempStr, strlen(tempStr), &w, &h);
    free(tempStr);
    
    return (Clay_Dimensions){(float)w, (float)h};
}

void ClayErrorHandler(Clay_ErrorData errorData) {
    SDL_Log("Clay Error: %.*s (Type: %d)", errorData.errorText.length, errorData.errorText.chars, errorData.errorType);
}

void HandleDropdownClick(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData) {
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        DropdownState *state = (DropdownState*)userData;
        state->isOpen = !state->isOpen;
    }
}

typedef struct {
    DropdownState* dropdown;
    I19C* lang_ctx;
} HoverContext;

void HandleOptionClick(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData) {
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        HoverContext* ctx = (HoverContext*)userData;
        DropdownState* state = ctx->dropdown;
        I19C* lang_state = ctx->lang_ctx;

        state->selectedIndex = elementId.offset;
        set_lang_i19c(lang_state, state->options[state->selectedIndex]);
        state->isOpen = false;
    }
}

void RenderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, strlen(text), color);
    if (!surface) return;
    
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_DestroySurface(surface);
        return;
    }
    
    SDL_FRect dstrect = {(float)x, (float)y, (float)surface->w, (float)surface->h};
    SDL_RenderTexture(renderer, texture, NULL, &dstrect);
    
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

int32_t main() {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Err: SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    if (!TTF_Init()) {
        SDL_Log("Err: TTF_Init failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    srand(time(NULL));

    Config *ctx = get_config();
	I19C *lang_ctx = get_i19c ();
	set_lang_i19c (lang_ctx, "english");

    SDL_Window *window = SDL_CreateWindow("Dropdown Example", ctx->width, ctx->height, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Err: SDL_CreateWindow failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, "software");
    if (!renderer) {
        SDL_Log("Err: SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("assets/font/NotoSansTamil-VariableFont_wdth,wght.ttf", 16);

    TextRenderData textData = {
        .font = font,
        .renderer = renderer
    };

    uint32_t memorySize = Clay_MinMemorySize() * 3;
    void *memory = malloc(memorySize);
    if (!memory) {
        SDL_Log("Err: Failed to allocate memory for Clay");
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(memorySize, memory);
    Clay_Context *clayContext = Clay_Initialize(clayMemory, (Clay_Dimensions){(float)ctx->width, (float)ctx->height}, (Clay_ErrorHandler){ClayErrorHandler, NULL});
    if (!clayContext) {
        SDL_Log("Err: Clay_Initialize failed");
        free(memory);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Clay_SetMeasureTextFunction(MeasureText, &textData);
    Clay_SetDebugModeEnabled(false);

    const char *dropdownOptions[] = {
        "english",
        "tamil", 
    };
    
    DropdownState dropdownState = {
        .options = dropdownOptions,
        .optionCount = 2,
        .selectedIndex = 0,
        .isOpen = false
    };

	HoverContext hoverCtx = {
	    .dropdown = &dropdownState,
	    .lang_ctx = get_i19c()
	};
    
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                Clay_SetLayoutDimensions((Clay_Dimensions){(float)e.window.data1, (float)e.window.data2});
            } else if (e.type == SDL_EVENT_MOUSE_MOTION) {
                Clay_SetPointerState((Clay_Vector2){(float)e.motion.x, (float)e.motion.y}, e.motion.state & SDL_BUTTON_LMASK);
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    Clay_SetPointerState((Clay_Vector2){(float)e.button.x, (float)e.button.y}, true);
                }
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    Clay_SetPointerState((Clay_Vector2){(float)e.button.x, (float)e.button.y}, false);
                }
            }
        }

        Clay_BeginLayout();

        CLAY({
            .id = CLAY_ID("MainContainer"),
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                .padding = {32, 32, 32, 32},
                .childGap = 20,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
            },
            .backgroundColor = {230, 230, 250, 255}
        }) {
            CLAY({
                .id = CLAY_ID("ContentWrapper"),
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(300), CLAY_SIZING_FIT()},
                    .childGap = 16,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                }
            }) {
				/* I19C Implementation */
                char *greetings_text = T (lang_ctx, "GREETING");
                Clay_String greetings = {
                    .chars = greetings_text,
                    .length = strlen(greetings_text)
                };
                CLAY_TEXT(greetings, CLAY_TEXT_CONFIG({
                    .textColor = {0, 0, 0, 255},
                }));

                CLAY({
                    .id = CLAY_ID("DropdownButton"),
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(40)},
                        .padding = {12, 12, 8, 8},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                    },
                    .backgroundColor = {0, 0, 0, 255},
                    .cornerRadius = CLAY_CORNER_RADIUS(4)
                }) {
                    Clay_OnHover(HandleDropdownClick, (intptr_t)&dropdownState);
                    
                    CLAY_TEXT(CLAY_STRING("Select language!!!"), CLAY_TEXT_CONFIG({
                        .textColor = {255, 255, 255, 255},
                        .fontSize = 16
                    }));
                }

                if (dropdownState.isOpen) {
                    CLAY({
                        .id = CLAY_ID("DropdownOptions"),
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                        .floating = {
                            .attachTo = CLAY_ATTACH_TO_PARENT,
                            .attachPoints = {
                                .element = CLAY_ATTACH_POINT_LEFT_TOP,
                                .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM
                            },
                            .offset = {0, 4},
                            .zIndex = 1000
                        },
                        .backgroundColor = {255, 255, 255, 255},
                        .cornerRadius = CLAY_CORNER_RADIUS(4),
                        .border = {
                            .color = {200, 200, 200, 255},
                            .width = {1, 1, 1, 1}
                        }
                    }) {
                        for (int i = 0; i < dropdownState.optionCount; i++) {
                            Clay_Color bgColor = (i == dropdownState.selectedIndex) 
                                ? (Clay_Color){240, 240, 240, 255}
                                : (Clay_Color){255, 255, 255, 255};
                            
                            if (Clay_Hovered() && i != dropdownState.selectedIndex) {
                                bgColor = (Clay_Color){245, 245, 245, 255};
                            }
                            
                            CLAY({
                                .id = CLAY_IDI("DropdownOption", i),
                                .layout = {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(36)},
                                    .padding = {12, 12, 8, 8}
                                },
                                .backgroundColor = bgColor
                            }) {
								Clay_OnHover(HandleOptionClick, (intptr_t)&hoverCtx);
                                Clay_String optionStr = {
                                    .chars = dropdownState.options[i],
                                    .length = strlen(dropdownState.options[i])
                                };
                                CLAY_TEXT(optionStr, CLAY_TEXT_CONFIG({
                                    .textColor = {60, 60, 60, 255},
                                    .fontSize = 16
                                }));
                            }
                        }
                    }
                }
            }
        }

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        for (int32_t i = 0; i < renderCommands.length; i++) {
            Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&renderCommands, i);
            if (!cmd) continue;

            switch (cmd->commandType) {
                case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                    Clay_Color color = cmd->renderData.rectangle.backgroundColor;
                    SDL_SetRenderDrawColor(renderer, (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a);
                    SDL_FRect rect = {
                        cmd->boundingBox.x,
                        cmd->boundingBox.y,
                        cmd->boundingBox.width,
                        cmd->boundingBox.height
                    };
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                }
                case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                    Clay_Color color = cmd->renderData.border.color;
                    SDL_SetRenderDrawColor(renderer, (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a);
                    
                    SDL_FRect rect = {
                        cmd->boundingBox.x,
                        cmd->boundingBox.y,
                        cmd->boundingBox.width,
                        cmd->boundingBox.height
                    };
                    SDL_RenderRect(renderer, &rect);
                    break;
                }
                case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                    Clay_Color textColor = cmd->renderData.text.textColor;
                    SDL_Color sdlColor = {(Uint8)textColor.r, (Uint8)textColor.g, (Uint8)textColor.b, (Uint8)textColor.a};
                    
                    char *tempStr = malloc(cmd->renderData.text.stringContents.length + 1);
                    memcpy(tempStr, cmd->renderData.text.stringContents.chars, cmd->renderData.text.stringContents.length);
                    tempStr[cmd->renderData.text.stringContents.length] = '\0';
                    
                    RenderText(renderer, font, tempStr, (int)cmd->boundingBox.x, (int)cmd->boundingBox.y, sdlColor);
                    
                    free(tempStr);
                    break;
                }
                default:
                    break;
            }
        }

        SDL_RenderPresent(renderer);
    }

    free(clayMemory.memory);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
