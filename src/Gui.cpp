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
#include <stdarg.h>
#include "main.hpp"

static const int PANEL_HEIGHT = 7;
static const int BAR_WIDTH = 20;
static const int MSG_X = BAR_WIDTH + 2;
static const int MSG_HEIGHT = PANEL_HEIGHT - 1;

Gui::Gui() {
        con = new TCODConsole(engine.screenWidth, PANEL_HEIGHT);
}

Gui::~Gui() {
        delete con;
        log.clearAndDelete();
}

Gui::Message::Message(const char *text, const TCODColor &col) :
        text(strdup(text)), col(col) {
}

Gui::Message::~Message() {
        free(text);
}

void Gui::message(const TCODColor &col, const char *text, ...) {
        // Build the text
        va_list ap;
        char buf[128];
        va_start(ap, text);
        vsprintf(buf, text, ap);
        va_end(ap);

        char *lineBegin = buf;
        char *lineEnd;

        do {
                // Make room for new messages
                if (log.size() == MSG_HEIGHT) {
                        Message *toRemove = log.get(0);
                        log.remove(toRemove);
                        delete toRemove;
                }

                // Detect end of line
                lineEnd = strchr(lineBegin, '\n');

                if (lineEnd)
                        *lineEnd = '\0';

                // Add a new message to the log
                Message *msg = new Message(lineBegin, col);
                log.push(msg);

                // Go to next line
                lineBegin = lineEnd + 1;
        } while (lineEnd);
}

void Gui::render() {
        // Clear the GUI console
        con->setDefaultBackground(TCODColor::black);
        con->clear();

        // Draw the health bar
        renderBar(1, 1, BAR_WIDTH, "HP", engine.player->destructible->hp,
                engine.player->destructible->maxHp,
                TCODColor::lightRed, TCODColor::darkerRed);
        
        // Draw the message log
        int y = 1;
        float colorCoef = 0.4f;
        for (Message **it = log.begin(); it != log.end(); it++) {
                Message *message = *it;
                con->setDefaultForeground(message->col * colorCoef);
                con->print(MSG_X, y, message->text);
                y++;
                if (colorCoef < 1.0f)
                        colorCoef += 0.3f;
        }

        // Blit the GUI console on the root console
        TCODConsole::blit(con, 0, 0, engine.screenWidth, PANEL_HEIGHT,
                TCODConsole::root, 0, engine.screenHeight - PANEL_HEIGHT);
}

void Gui::renderBar(int x, int y, int width, const char *name, float value,
        float maxValue, const TCODColor &barColor,
        const TCODColor &backColor) {
        // Fill the background
        con->setDefaultBackground(backColor);
        con->rect(x, y, width, 1, false, TCOD_BKGND_SET);

        int barWidth = (int)(value / maxValue * width);
        if (barWidth > 0) {
                // Draw the bar
                con->setDefaultBackground(barColor);
                con->rect(x, y, barWidth, 1, false, TCOD_BKGND_SET);
        }

        // Print text on top of the bar
        con->setDefaultForeground(TCODColor::white);
        con->printEx(x + width/2, y, TCOD_BKGND_NONE, TCOD_CENTER,
                "%s : %g/%g", name, value, maxValue);
}
