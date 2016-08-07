/* Copyright (c) 2016 Sylvia van Os
 * Copyright (c) 2008,2009,2010,2012 Jice & Mingos
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Jice or Mingos may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JICE AND MINGOS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL JICE OR MINGOS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <sys/time.h>
#include "main.hpp"

Engine::Engine(int screenWidth, int screenHeight) : gameStatus(STARTUP),
        fovRadius(10), screenWidth(screenWidth), screenHeight(screenHeight) {
        TCODConsole::initRoot(screenWidth, screenHeight, "The Crispy Broccoli Adventures", false);
        player = new Actor(40, 25, '@', "player", TCODColor::white);
        player->destructible = new PlayerDestructible(30, 2, "your cadaver");
        player->attacker = new Attacker(1);
        player->ai = new PlayerAi();
        actors.push(player);
        map = new Map(80, 43);
        gui = new Gui();
        gui->message(TCODColor::red,
                "Find the crispy piece of broccoli and escape the dungeon.");
}

Engine::~Engine() {
        actors.clearAndDelete();
        delete map;
        delete gui;
}

void Engine::sendToBack(Actor *actor) {
        actors.remove(actor);
        actors.insertBefore(actor, 0);
}

void Engine::update() {
        lastKey = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);

        if (gameStatus == STARTUP) {
                computeFov = true;
                gameStatus = PLAYING;
        }

        if (gameStatus == VICTORY || gameStatus == DEFEAT)
                return;

        if (computeFov) {
                map->computeFov();
                computeFov = false;
        }

        for (Actor **iterator = actors.begin(); iterator != actors.end();
                iterator++) {
                Actor *actor = *iterator;
                actor->update();
        }
}

void Engine::render() {
        TCODConsole::root->clear();
        map->render();

        for (Actor **iterator = actors.begin();
                iterator != actors.end(); iterator++) {
                Actor *actor = *iterator;
                if (map->isInFov(actor->x, actor->y)) {
                        actor->render();
                }
        }

        player->render();
        // Show player stats
        gui->render();
}
