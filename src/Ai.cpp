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
#include <math.h>
#include "main.hpp"

void PlayerAi::update(Actor *owner) {
        if (owner->destructible && owner->destructible->isDead()) {
                return;
        }

        int dx = 0, dy = 0;

        if (engine.lastKey.vk == TCODK_UP || engine.lastKey.c == 'k')
                dy = -1;
        else if (engine.lastKey.vk == TCODK_DOWN || engine.lastKey.c == 'j')
                dy = 1;
        else if (engine.lastKey.vk == TCODK_LEFT || engine.lastKey.c == 'h')
                dx = -1;
        else if (engine.lastKey.vk == TCODK_RIGHT || engine.lastKey.c == 'l')
                dx = 1;

        if (dx != 0 || dy != 0) {
                if (moveOrAttack(owner, owner->x + dx, owner->y + dy))
                        engine.computeFov = true;
        }

        if ((int)TCODSystem::getElapsedMilli() < engine.player->blinded) {
                // compute empty Fov while blinded
                engine.computeFov = false;
                engine.map->emptyFov();
        } else if (engine.player->blinded > 0) {
                // Force computeFov once when blind ends
                engine.computeFov = true;
                engine.player->blinded = 0;
        }
}

bool PlayerAi::moveOrAttack(Actor *owner, int targetx, int targety) {
        int timeStamp = TCODSystem::getElapsedMilli();

        // Can't move when frozen
        if (timeStamp < owner->frozen)
                return false;

        if (engine.map->isWall(targetx, targety))
                return false;

        // Look for destructibles to attack
        for (Actor **iterator = engine.actors.begin();
                iterator != engine.actors.end(); iterator++) {
                Actor *actor = *iterator;
                if (actor->destructible && !actor->destructible->isDead()
                        && actor->x == targetx && actor->y == targety) {
                        owner->attacker->attack(owner, actor);

                        return false;
                }
        }

        // Look for corpses
        for (Actor **iterator = engine.actors.begin();
                iterator != engine.actors.end(); iterator++) {
                Actor *actor = *iterator;
                if (actor->destructible && actor->destructible->isDead()
                        && actor->x == targetx && actor->y == targety) {
                        engine.gui->message(TCODColor::lightGrey,
                                "There is a %s here.", actor->name);
                }
        }

        owner->x = targetx;
        owner->y = targety;
        
        return true;
}

void MonsterAi::update(Actor *owner) {
        if (owner->destructible && owner->destructible->isDead())
                return;

        moveOrAttack(owner, engine.player->x, engine.player->y);
}

void MonsterAi::moveOrAttack(Actor *owner, int targetx, int targety) {
        int timeStamp = TCODSystem::getElapsedMilli();

        // Can't move when frozen or blinded
        if (timeStamp < owner->frozen || timeStamp < owner->blinded)
                return;

        // Update once every quarter second
        if (timeStamp - 250 < this->lastMove)
                return;

        // Don't move if not in Fov unless player is blinded
        if (timeStamp > engine.player->blinded && !engine.map->isInFov(owner->x, owner-> y))
                return;

        this->lastMove = timeStamp;

        int dx = targetx - owner->x;
        int dy = targety - owner->y;
        int stepdx = (dx > 0 ? 1 : -1);
        int stepdy = (dy > 0 ? 1 : -1);
        float distance = sqrtf(dx*dx + dy*dy);

        if (distance >= 2) {
                dx = (int)(round(dx/distance));
                dy = (int)(round(dy/distance));

                if (engine.map->canWalk(owner->x + dx, owner->y + dy) &&
                        !engine.map->shouldAvoid(owner->x + dx,
                        owner->y + dy)) {
                        owner->x += dx;
                        owner->y += dy;
                } else if (engine.map->canWalk(owner->x + stepdx, owner->y) &&
                        !engine.map->shouldAvoid(owner->x + stepdx, owner->y)) {
                        owner->x += stepdx;
                } else if (engine.map->canWalk(owner->x, owner->y + stepdy) &&
                        !engine.map->shouldAvoid(owner->x, owner->y + stepdy)) {
                        owner->y += stepdy;
                }
        } else if (owner->attacker) {
                owner->attacker->attack(owner, engine.player);
        }
}

void BroccoliAi::update(Actor *owner) {
        if (!owner->hidden && engine.player->x == owner->x && engine.player->y == owner->y) {
                engine.gui->message(TCODColor::lightGreen,
                        "%s has found %s.", engine.player->name, owner->name);
                engine.player->hasBroccoli = true;
                owner->hidden = true;
        }
}

void FakeBroccoliAi::update(Actor *owner) {
        if (!owner->hidden && engine.player->x == owner->x && engine.player->y == owner->y) {
                engine.gui->message(TCODColor::yellow,
                        "%s has found %s.", engine.player->name, owner->name);
                engine.gui->message(TCODColor::yellow,
                        "%s throws %s away.", engine.player->name, owner->name);
                owner->hidden = true;
        }
}

void ExitAi::update(Actor *owner) {
        if (engine.player->x == owner->x && engine.player->y == owner->y &&
                engine.player->hasBroccoli) {
                engine.gui->message(TCODColor::green,
                        "%s has left the dungeon.", engine.player->name);
                engine.gui->message(TCODColor::green,
                        "%s was victorious.", engine.player->name);
                engine.gameStatus = engine.VICTORY;
        }
}

void BlindTrapAi::update(Actor *owner) {
        // Only blind once every 1.1 seconds
        int timeStamp = TCODSystem::getElapsedMilli();

        if (timeStamp - 1100 < this->lastMove)
                return;

        // Blind whatever enemy is on this spot for 1 second
        for (Actor **iterator = engine.actors.begin();
                iterator != engine.actors.end(); iterator++) {
                Actor *actor = *iterator;
                if (actor->destructible && !actor->destructible->isDead()
                        && actor->x == owner->x && actor->y == owner->y) {
                        // Trap was found
                        owner->hidden = false;

                        engine.gui->message(TCODColor::lightGrey,
                                "%s stepped on %s.",
                                actor->name, owner->name);
                        actor->blinded = timeStamp + 1000;
                        this->lastMove = timeStamp;
                }
        }
}

void FreezeTrapAi::update(Actor *owner) {
        // Only freeze once every 1.1 seconds
        int timeStamp = TCODSystem::getElapsedMilli();

        if (timeStamp - 1100 < this->lastMove)
                return;

        // Freeze whatever enemy is on this spot for 1 second
        for (Actor **iterator = engine.actors.begin();
                iterator != engine.actors.end(); iterator++) {
                Actor *actor = *iterator;
                if (actor->destructible && !actor->destructible->isDead()
                        && actor->x == owner->x && actor->y == owner->y) {
                        // Trap was found
                        owner->hidden = false;

                        engine.gui->message(TCODColor::lightGrey,
                                "%s stepped on %s.",
                                actor->name, owner->name);
                        actor->frozen = timeStamp + 1000;
                        this->lastMove = timeStamp;
                }
        }
}

void TeleporterTrapAi::update(Actor *owner) {
        for (Actor **iterator = engine.actors.begin();
                iterator != engine.actors.end(); iterator++) {
                Actor *actor = *iterator;
                if (actor->destructible && actor->x == owner->x &&
                        actor->y == owner->y) {
                        // Trap was found
                        owner->hidden = false;

                        // Find a place to teleport them too
                        while (true) {
                                TCODRandom *rng = TCODRandom::getInstance();
                                int x = rng->getInt(0, engine.map->width);
                                int y = rng->getInt(0, engine.map->height);
                                
                                if (engine.map->canWalk(x, y)) {
                                        engine.gui->message(TCODColor::lightGrey,
                                                "%s stepped on %s.",
                                                actor->name, owner->name);
                                        actor->x = x;
                                        actor->y = y;
                                        break;
                                }
                        }
                }
        }
}
