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

#include "main.hpp"

static const int ROOM_MAX_SIZE = 12;
static const int ROOM_MIN_SIZE = 6;
static const int MAX_ROOM_MONSTERS = 2;
static const int MAX_TRAPS = 30;

class BspListener : public ITCODBspCallback {
private :
        Map &map; // A map to dig
        int roomNum; // Room number
        int lastx, lasty; // Center of the last room

public :
        BspListener(Map &map) : map(map), roomNum(0) {}
        bool visitNode(TCODBsp *node, void *userData) {
                if (node->isLeaf()) {
                        int x, y, w, h;
                        // Dig a room
                        TCODRandom *rng = TCODRandom::getInstance();
                        w = rng->getInt(ROOM_MIN_SIZE, node->w - 2);
                        h = rng->getInt(ROOM_MIN_SIZE, node->h - 2);
                        x = rng->getInt(node->x + 1, node->x + node->w - w - 1);
                        y = rng->getInt(node->y + 1, node->y + node->h - h - 1);
                        map.createRoom(roomNum == 0, x, y, x + w - 1, y + h - 1);

                        if (roomNum != 0) {
                                // Dig a corridor from last room
                                map.dig(lastx, lasty, x + w / 2, lasty);
                                map.dig(x + w / 2, lasty, x + w / 2, y + h / 2);
                        }

                        lastx = x + w / 2;
                        lasty = y + h / 2;
                        roomNum++;
                }

                return true;
        }
};

Map::Map(int width, int height) : width(width), height(height) {
        tiles = new Tile[width * height];
        map = new TCODMap(width, height);
        TCODBsp bsp(0, 0, width, height);
        bsp.splitRecursive(NULL, 8, ROOM_MAX_SIZE, ROOM_MAX_SIZE, 1.5f, 1.5f);
        BspListener listener(*this);
        bsp.traverseInvertedLevelOrder(&listener, NULL);

        TCODRandom *rng = TCODRandom::getInstance();

        // Place crispy broccoli somewhere
        while (true) {
                int x = rng->getInt(0, width);
                int y = rng->getInt(0, height);
                
                if (canWalk(x, y)) {
                        Actor *broccoli = new Actor(x, y, '!', "a piece of crispy broccoli", TCODColor::green);
                        broccoli->ai = new BroccoliAi();
                        engine.actors.push(broccoli);
                        break;
                }
        }
        
        // Place non-crispy broccoli somewhere with 10% chance
        if (rng->getInt(0, 9) == 0) {
                while (true) {
                        int x = rng->getInt(0, width);
                        int y = rng->getInt(0, height);
                        
                        if (canWalk(x, y)) {
                                Actor *fakeBroccoli = new Actor(x, y, '!', "a piece of regular broccoli", TCODColor::green);
                                fakeBroccoli->ai = new FakeBroccoliAi();
                                engine.actors.push(fakeBroccoli);
                                break;
                        }
                }
        }

        // Place traps
        int nbTraps = rng->getInt(0, MAX_TRAPS);
        while (nbTraps > 0) {
                int x = rng->getInt(0, width);
                int y = rng->getInt(0, height);

                if (canWalk(x, y)) {
                        addTrap(x, y);
                }
                nbTraps--;
        }
}

Map::~Map() {
        delete [] tiles;
        delete map;
}

bool Map::canWalk(int x, int y) const {
        if (isWall(x, y)) {
                // There is a wall here
                return false;
        }

        for (Actor **iterator = engine.actors.begin();
                iterator != engine.actors.end(); iterator++) {
                Actor *actor = *iterator;
                if (actor->blocks && actor->x == x && actor->y == y) {
                        // Another blocking actor is here
                        return false;
                }
        }

        return true;
}

bool Map::shouldAvoid(int x, int y) const {
        for (Actor **iterator = engine.actors.begin();
                iterator != engine.actors.end(); iterator++) {
                Actor *actor = *iterator;

                if (actor->avoid && actor->x == x && actor->y == y && actor->hidden) {
                        // A visible avoidable is here
                        return true;
                }
        }

        return false;
}        

bool Map::isWall(int x, int y) const {
        return !map->isWalkable(x, y);
}

bool Map::isExplored(int x, int y) const {
        return tiles[x + y*width].explored;
}

bool Map::isInFov(int x, int y) const {
        if (map->isInFov(x, y)) {
                tiles[x + y * width].explored = true;
                return true;
        }

        return false;
}

void Map::computeFov() {
        map->computeFov(engine.player->x, engine.player->y, engine.fovRadius);
}

void Map::emptyFov() {
        map->computeFov(-1, -1, 1);
}

void Map::addMonster(int x, int y) {
        Actor *grue = new Actor(x, y, '#', "grue", TCODColor::red);
        grue->destructible = new MonsterDestructible(15, 0, "dead grue");
        grue->attacker = new Attacker(3);
        grue->ai = new MonsterAi();
        engine.actors.push(grue);
}

void Map::addTrap(int x, int y) {
        TCODRandom *rng = TCODRandom::getInstance();
        Actor *trap;
        int trapId = rng->getInt(0, 2);
        if (trapId == 0) {
                trap = new Actor(x, y, '^', "teleporter trap", TCODColor::yellow);
                trap->ai = new TeleporterTrapAi();
        } else if(trapId == 1) {
                trap = new Actor(x, y, '^', "freeze trap", TCODColor::lightBlue);
                trap->ai = new FreezeTrapAi();
        } else {
                trap = new Actor(x, y, '^', "blinding trap", TCODColor::black);
                trap->ai = new BlindTrapAi();
        }
        trap->avoid = true;
        trap->blocks = false;
        trap->hidden = true;
        engine.actors.push(trap);
}

void Map::dig(int x1, int y1, int x2, int y2) {
        if (x2 < x1) {
                int tmp = x2;
                x2 = x1;
                x1 = tmp;
        }

        if (y2 < y1) {
                int tmp = y2;
                y2 = y1;
                y1 = tmp;
        }

        for (int tilex = x1; tilex <= x2; tilex++) {
                for (int tiley = y1; tiley <= y2; tiley++) {
                        map->setProperties(tilex, tiley, true, true);
                }
        }
}

void Map::createRoom(bool first, int x1, int y1, int x2, int y2) {
        dig(x1, y1, x2, y2);

        if (first) {
                // Put the player in the first room
                engine.player->x = (x1 + x2) / 2;
                engine.player->y = (y1 + y2) / 2;
                // Put the exit on the same spot
                Actor *exit = new Actor(engine.player->x, engine.player->y,
                        '~', "exit", TCODColor::lightGrey);
                exit->blocks = false;
                exit->ai = new ExitAi();
                engine.actors.push(exit);

        } else {
                TCODRandom *rng = TCODRandom::getInstance();
                int nbMonsters = rng->getInt(0, MAX_ROOM_MONSTERS);
                while (nbMonsters > 0) {
                        int x = rng->getInt(x1, x2);
                        int y = rng->getInt(y1, y2);

                        if (canWalk(x, y)) {
                                addMonster(x, y);
                        }
                        nbMonsters--;
                }
        }
}

void Map::render() const {
        static const TCODColor darkWall(0, 0, 100);
        static const TCODColor darkGround(50, 50, 150);
        static const TCODColor lightWall(130, 110, 50);
        static const TCODColor lightGround(200, 180, 50);

        for (int x = 0; x < width; x++) {
                for (int y = 0; y < height; y++) {
                        if (isInFov(x, y)) {
                                TCODConsole::root->setCharBackground(x, y,
                                        isWall(x, y) ? lightWall : lightGround);
                        } else if (isExplored(x, y)) {
                                TCODConsole::root->setCharBackground(x, y,
                                        isWall(x, y) ? darkWall : darkGround);
                        }
                }
        }
}
