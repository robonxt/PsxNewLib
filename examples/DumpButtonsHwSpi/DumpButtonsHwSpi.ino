/*******************************************************************************
 * This file is part of PsxNewLib.                                             *
 *                                                                             *
 * Copyright (C) 2019-2020 by SukkoPera <software@sukkology.net>               *
 *                                                                             *
 * PsxNewLib is free software: you can redistribute it and/or                  *
 * modify it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or           *
 * (at your option) any later version.                                         *
 *                                                                             *
 * PsxNewLib is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License           *
 * along with PsxNewLib. If not, see http://www.gnu.org/licenses.              *
 *******************************************************************************
 *
 * This sketch will dump to serial whatever is done on a PSX controller. It is
 * an excellent way to test that all buttons/sticks are read correctly.
 *
 * It's missing support for analog buttons, that will come in the future.
 *
 * This example drives the controller through the hardware SPI port, so pins are
 * fixed and depend on the board/microcontroller being used. For instance, on an
 * Arduino Uno connections must be as follows:
 *
 * CMD: Pin 11
 * DATA: Pin 12
 * CLK: Pin 13
 *
 * Any pin can be used for ATTN, but please note that most 8-bit AVRs require
 * the HW SPI SS pin to be kept as an output for HW SPI to be in master mode, so
 * using that pin for ATTN is a natural choice. On the Uno this would be pin 10.
 *
 * It also works perfectly on OpenPSX2AmigaPadAdapter boards (as it's basically
 * a modified Uno).
 *
 * There is another similar one using a bitbanged protocol implementation that
 * can be used on any pins/board.
 */

#include <DigitalIO.h>
#include <PsxControllerHwSpi.h>

#include <avr/pgmspace.h>
typedef const __FlashStringHelper * FlashStr;
typedef const byte* PGM_BYTES_P;
#define PSTR_TO_F(s) reinterpret_cast<const __FlashStringHelper *> (s)

// This can be changed freely but please see above
const byte PIN_PS2_ATT = 10;

const byte PIN_BUTTONPRESS = A0;
const byte PIN_HAVECONTROLLER = A1;

const char buttonSelectName[] PROGMEM = "Select";
const char buttonL3Name[] PROGMEM = "L3";
const char buttonR3Name[] PROGMEM = "R3";
const char buttonStartName[] PROGMEM = "Start";
const char buttonUpName[] PROGMEM = "Up";
const char buttonRightName[] PROGMEM = "Right";
const char buttonDownName[] PROGMEM = "Down";
const char buttonLeftName[] PROGMEM = "Left";
const char buttonL2Name[] PROGMEM = "L2";
const char buttonR2Name[] PROGMEM = "R2";
const char buttonL1Name[] PROGMEM = "L1";
const char buttonR1Name[] PROGMEM = "R1";
const char buttonTriangleName[] PROGMEM = "Triangle";
const char buttonCircleName[] PROGMEM = "Circle";
const char buttonCrossName[] PROGMEM = "Cross";
const char buttonSquareName[] PROGMEM = "Square";

const char* const psxButtonNames[PSX_BUTTONS_NO] PROGMEM = {
	buttonSelectName,
	buttonL3Name,
	buttonR3Name,
	buttonStartName,
	buttonUpName,
	buttonRightName,
	buttonDownName,
	buttonLeftName,
	buttonL2Name,
	buttonR2Name,
	buttonL1Name,
	buttonR1Name,
	buttonTriangleName,
	buttonCircleName,
	buttonCrossName,
	buttonSquareName
};

byte psxButtonToIndex (PsxButtons psxButtons) {
	byte i;

	for (i = 0; i < PSX_BUTTONS_NO; ++i) {
		if (psxButtons & 0x01) {
			break;
		}

		psxButtons >>= 1U;
	}

	return i;
}

FlashStr getButtonName (PsxButtons psxButton) {
	FlashStr ret = F("");
	
	byte b = psxButtonToIndex (psxButton);
	if (b < PSX_BUTTONS_NO) {
		PGM_BYTES_P bName = reinterpret_cast<PGM_BYTES_P> (pgm_read_ptr (&(psxButtonNames[b])));
		ret = PSTR_TO_F (bName);
	}

	return ret;
}

void dumpButtons (PsxButtons psxButtons) {
	static PsxButtons lastB = 0;

	if (psxButtons != lastB) {
		lastB = psxButtons;     // Save it before we alter it
		
		Serial.print (F("Pressed: "));

		for (byte i = 0; i < PSX_BUTTONS_NO; ++i) {
			byte b = psxButtonToIndex (psxButtons);
			if (b < PSX_BUTTONS_NO) {
				PGM_BYTES_P bName = reinterpret_cast<PGM_BYTES_P> (pgm_read_ptr (&(psxButtonNames[b])));
				Serial.print (PSTR_TO_F (bName));
			}

			psxButtons &= ~(1 << b);

			if (psxButtons != 0) {
				Serial.print (F(", "));
			}
		}

		Serial.println ();
	}
}

void dumpAnalog (const char *str, const int8_t x, const int8_t y) {
	Serial.print (str);
	Serial.print (F(" analog: x = "));
	Serial.print (x);
	Serial.print (F(", y = "));
	Serial.println (y);
}



const char ctrlTypeUnknown[] PROGMEM = "Unknown";
const char ctrlTypeDualShock[] PROGMEM = "Dual Shock";
const char ctrlTypeDsWireless[] PROGMEM = "Dual Shock Wireless";
const char ctrlTypeGuitHero[] PROGMEM = "Guitar Hero";
const char ctrlTypeOutOfBounds[] PROGMEM = "(Out of bounds)";

const char* const controllerTypeStrings[PSCTRL_MAX + 1] PROGMEM = {
	ctrlTypeUnknown,
	ctrlTypeDualShock,
	ctrlTypeDsWireless,
	ctrlTypeGuitHero,
	ctrlTypeOutOfBounds
};







PsxControllerHwSpi<PIN_PS2_ATT> psx;

boolean haveController = false;
 
void setup () {
	fastPinMode (PIN_BUTTONPRESS, OUTPUT);
	fastPinMode (PIN_HAVECONTROLLER, OUTPUT);
	
	delay (300);

	Serial.begin (115200);
	Serial.println (F("Ready!"));
}
 
void loop () {
	static int8_t slx, sly, srx, sry;
	
	fastDigitalWrite (PIN_HAVECONTROLLER, haveController);
	
	if (!haveController) {
		if (psx.begin ()) {
			Serial.println (F("Controller found!"));
			delay (300);
			if (!psx.enterConfigMode ()) {
				Serial.println (F("Cannot enter config mode"));
			} else {
				PsxControllerType ctype = psx.getControllerType ();
				PGM_BYTES_P cname = reinterpret_cast<PGM_BYTES_P> (pgm_read_ptr (&(controllerTypeStrings[ctype < PSCTRL_MAX ? static_cast<byte> (ctype) : PSCTRL_MAX])));
				Serial.print (F("Controller Type is: "));
				Serial.println (PSTR_TO_F (cname));

				if (!psx.enableAnalogSticks ()) {
					Serial.println (F("Cannot enable analog sticks"));
				}
				
				//~ if (!psx.setAnalogMode (false)) {
					//~ Serial.println (F("Cannot disable analog mode"));
				//~ }
				//~ delay (10);
				
				if (!psx.enableAnalogButtons ()) {
					Serial.println (F("Cannot enable analog buttons"));
				}
				
				if (!psx.exitConfigMode ()) {
					Serial.println (F("Cannot exit config mode"));
				}
			}
			
			haveController = true;
		}
	} else {
		if (!psx.read ()) {
			Serial.println (F("Controller lost :("));
			haveController = false;
		} else {
			fastDigitalWrite (PIN_BUTTONPRESS, !!psx.getButtonWord ());
			dumpButtons (psx.getButtonWord ());

			byte lx, ly;
			psx.getLeftAnalog (lx, ly);
			if (lx != slx || ly != sly) {
				dumpAnalog ("Left", lx, ly);
				slx = lx;
				sly = ly;
			}

			byte rx, ry;
			psx.getRightAnalog (rx, ry);
			if (rx != srx || ry != sry) {
				dumpAnalog ("Right", rx, ry);
				srx = rx;
				sry = ry;
			}
		}
	}

	
	delay (1000 / 60);
}
