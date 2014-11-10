/*
 * Display.cpp
 *
 * Created: 04/11/2014 09:42:47
 *  Author: David
 */ 

#include "Display.hpp"
#include <algorithm>

extern UTFT lcd;
extern uint8_t glcd19x20[];

extern void WriteCommand(const char* array s);
extern void WriteCommand(int i);
extern void WriteCommand(char c);


// Static fields of class DisplayField
LcdFont DisplayField::defaultFont = glcd19x20;
LcdColour DisplayField::defaultFcolour = 0xFFFF;
LcdColour DisplayField::defaultBcolour = 0;

// Find the best match to a touch event in a list of fields
DisplayField * null DisplayField::FindEvent(int x, int y, DisplayField * null p)
{	
	const int maxXerror = 10, maxYerror = 10;		// set these to how close we need to be
	int bestError = maxXerror + maxYerror;
	DisplayField * null best = NULL;
	while (p != NULL)
	{
		if (p->GetEvent() != nullEvent)
		{
			int xError = (x < (int)p->GetMinX()) ? (int)p->GetMinX() - x
									: (x > (int)p->GetMaxX()) ? x - (int)p->GetMaxX()
										: 0;
			if (xError < maxXerror)
			{
				int yError = (y < (int)p->GetMinY()) ? (int)p->GetMinY() - y
										: (y > (int)p->GetMaxY()) ? y - (int)p->GetMaxY()
											: 0;
				if (yError < maxYerror && xError + yError < bestError)
				{
					bestError = xError + yError;
					best = p;
				}
			}
		}
		p = p->next;
	}
	return best;
}

DisplayManager::DisplayManager()
	: backgroundColor(0), root(NULL), popupField(NULL)
{
}

void DisplayManager::Init(LcdColour bc)
{
	backgroundColor = bc;
	lcd.fillScr(backgroundColor);
}

// Append a field to the list of displayed fields
void DisplayManager::AddField(DisplayField *d)
{
	d->next = root;
	root = d;
}

// Refresh all fields. if 'full' is true then we rewrite them all, else we just rewrite those that have changed.
void DisplayManager::RefreshAll(bool full)
{
	for (DisplayField * null pp = root; pp != NULL; pp = pp->next)
	{
		if (full || Visible(pp))
		{
			pp->Refresh(full, 0, 0);
		}		
	}
	if (HavePopup())
	{
		popupField->Refresh(full, popupX, popupY);
	}
}

bool DisplayManager::Visible(const DisplayField *p) const
{
	return !HavePopup()
			|| (   p->GetMaxY() < popupY || p->GetMinY() >= popupY + popupField->GetHeight()
				|| p->GetMaxX() < popupX || p->GetMinX() >= popupX + popupField->GetWidth()
			   );
}

// Get the event the corresponds to the field that has been touched, or nullEvent if we can't find one
DisplayField * null DisplayManager::FindEvent(PixelNumber x, PixelNumber y)
{
	return (HavePopup()) ? popupField->FindEvent((int)x - (int)popupX, (int)y - (int)popupY) : DisplayField::FindEvent((int)x, (int)y, root);
}

void DisplayManager::SetPopup(PopupField * null p, PixelNumber px, PixelNumber py)
{
	if (popupField != p)
	{		
		if (popupField != NULL)
		{
			lcd.setColor(backgroundColor);
			lcd.fillRect(popupX, popupY, popupX + popupField->GetWidth() - 1, popupY + popupField->GetHeight() - 1);
			
			// Re-display the background fields
			for (DisplayField * null pp = root; pp != NULL; pp = pp->next)
			{
				if (!Visible(pp))
				{
					pp->Refresh(true, 0, 0);
				}
			}		
		}
		popupField = p;
		if (p != NULL)
		{
			popupX = px;
			popupY = py;
			p->Refresh(true, popupX, popupY);
		}
	}
}

void DisplayManager::AttachPopup(PopupField * pp, DisplayField *p)
{
	// Work out the Y coordinate to place the popup level with the field
	PixelNumber h = pp->GetHeight()/2;
	PixelNumber hy = (p->GetMinY() + p->GetMaxY() + 1)/2;
	PixelNumber y = (hy + h > lcd.getDisplayYSize()) ? lcd.getDisplayYSize() - pp->GetHeight()
					: (hy > h) ? hy - h 
						: 0;
	
	PixelNumber x = (p->GetMaxX() + 5 + pp->GetWidth() < lcd.getDisplayXSize()) ? p->GetMaxX() + 5
						: p->GetMinX() - pp->GetWidth() - 5;
	SetPopup(pp, x, y);
}

// Set the font and colours, print the label (if any) and leave the cursor at the correct position for the data
void LabelledField::DoLabel(bool full, PixelNumber xOffset, PixelNumber yOffset)
pre(full || changed)
{
	lcd.setFont(font);
	lcd.setColor(fcolour);
	lcd.setBackColor(bcolour);
	if (label != NULL && (full || changed))
	{
		lcd.setTextPos(x + xOffset, y + yOffset, x + xOffset + width);
		lcd.print(label);
		labelColumns = lcd.getTextX() + 1 - x - xOffset;
	}
	else
	{
		lcd.setTextPos(x + xOffset + labelColumns, y + yOffset, x + xOffset + width);
	}
}

void TextField::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		DoLabel(full, xOffset, yOffset);
		lcd.print(text);
		lcd.clearToMargin();
		changed = false;
	}
}

void FloatField::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		DoLabel(full, xOffset, yOffset);
		lcd.setTranslation(".", "\x80");
		lcd.print(val, numDecimals);
		lcd.setTranslation(NULL, NULL);
		if (units != NULL)
		{
			lcd.print(units);
		}
		lcd.clearToMargin();
		changed = false;
	}
}

void IntegerField::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		DoLabel(full, xOffset, yOffset);
		lcd.setTranslation(".", "\x16");
		lcd.print(val);
		lcd.setTranslation(NULL, NULL);
		if (units != NULL)
		{
			lcd.print(units);
		}
		lcd.clearToMargin();
		changed = false;
	}
}

void StaticTextField::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		lcd.setFont(font);
		lcd.setColor(fcolour);
		lcd.setBackColor(bcolour);
		lcd.setTextPos(x + xOffset, y + yOffset, x + xOffset + width);
		if (align == Left)
		{
			lcd.print(text);
			lcd.clearToMargin();
		}
		else
		{
			lcd.clearToMargin();
			lcd.setTextPos(0, 9999, width);
			lcd.print(text);    // dummy print to get text width
			PixelNumber spare = width - lcd.getTextX();
			lcd.setTextPos(x + xOffset + ((align == Centre) ? spare/2 : spare), y + yOffset, x + xOffset + width);
			lcd.print(text);
		}
		changed = false;
	}
}

void ProgressBar::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		PixelNumber pixelsSet = ((width - 2) * percent)/100;
		if (full)
		{
			lcd.setColor(fcolour);
			lcd.drawLine(x + xOffset, y, x + xOffset + width - 1, y + yOffset);
			lcd.drawLine(x + xOffset, y + yOffset + height - 1, x + xOffset + width - 1, y + yOffset + height - 1);
			lcd.drawLine(x + xOffset + width - 1, y + yOffset + 1, x + xOffset + width - 1, y + yOffset + height - 2);

			lcd.fillRect(x + xOffset, y + yOffset + 1, x + xOffset + pixelsSet, y + yOffset + height - 2);
			if (pixelsSet < width - 2)
			{
				lcd.setColor(bcolour);
				lcd.fillRect(x + xOffset + pixelsSet + 1, y + yOffset + 1, x + xOffset + width - 2, y + yOffset + height - 2);
			}
		}
		else if (pixelsSet > lastNumPixelsSet)
		{
			lcd.setColor(fcolour);
			lcd.fillRect(x + xOffset + lastNumPixelsSet, y + yOffset + 1, x + xOffset + pixelsSet, y + yOffset + height - 2);
		}
		else if (pixelsSet < lastNumPixelsSet)
		{
			lcd.setColor(bcolour);
			lcd.fillRect(x + xOffset + pixelsSet + 1, y + yOffset + 1, x + xOffset + lastNumPixelsSet, y + yOffset + height - 2);	
		}
		changed = false;
		lastNumPixelsSet = pixelsSet;
	}
}

PopupField::PopupField(PixelNumber ph, PixelNumber pw)
	: height(ph), width(pw), root(NULL)
{
}

void PopupField::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset) override
{
	if (full)
	{
		// Draw a black border
		lcd.setColor(black);
		lcd.drawRect(xOffset, yOffset, xOffset + width - 1, yOffset + height - 1);
		
		// Draw a rectangle inside that one
		lcd.setColor(green);
		lcd.fillRect(xOffset + 1, yOffset + 1, xOffset + width - 2, yOffset + height - 2);
	}
	
	for (DisplayField * null p = root; p != NULL; p = p->next)
	{
		p->Refresh(full, xOffset, yOffset);
	}
}

void PopupField::AddField(DisplayField *p)
{
	p->next = root;
	root = p;	
}

DisplayField *PopupField::FindEvent(int px, int py)
{
	return DisplayField::FindEvent(px, py, root);
}

IntegerSettingField::IntegerSettingField(const char* array pc, PixelNumber py, PixelNumber px, PixelNumber pw, const char *pl, const char *pu)
	: IntegerField(py, px, pw, pl, pu), cmd(pc)
{
}

void IntegerSettingField::Action()
{
	WriteCommand(cmd);
	WriteCommand(GetValue());
	WriteCommand('\n');
}

// End