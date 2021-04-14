#include "FileSection.h"
static juce::Image fileImage(juce::Image::PixelFormat::ARGB, 600, 250, true);

RangeHandle::RangeHandle(juce::AudioProcessorValueTreeState& vts, SharedFileBuffer* buf) : fileBuffer(buf), startAtt(*vts.getParameter("Range Start"), 
	[this, &vts](float value) 
	{
		int numSamples = fileBuffer->buffer.getNumSamples();
		if (value >= numSamples) value = numSamples - 1;
		contexts[0].relativeX = value / fileBuffer->buffer.getNumSamples();
		//contexts[0].relativeX = vts.getParameter("Range Start")->convertTo0to1(f);
		resized(); repaint();
	}),
																	endAtt(*vts.getParameter("Range End"), 
	[this, &vts](float value) 
	{
		int numSamples = fileBuffer->buffer.getNumSamples();
		if (value >= numSamples) value = numSamples - 1;
		contexts[1].relativeX = value / fileBuffer->buffer.getNumSamples();
		resized(); repaint();
	})
{
	startAtt.sendInitialUpdate();
	endAtt.sendInitialUpdate();
	contexts[0].relativeX = 0.0f;
	contexts[1].relativeX = 1.0f;
	contexts[0].range = vts.getParameterRange("Range Start");
	contexts[1].range = vts.getParameterRange("Range End");
}
void RangeHandle::paint(juce::Graphics& g)
{
		g.setColour(juce::Colours::red);
		g.setOpacity(0.7f);


		auto width = getWidth();
		int actualX[2] = { contexts[0].relativeX * width, contexts[1].relativeX * (width-5) };
		g.setOpacity(0.1f);
		g.fillRect(juce::Rectangle<int>(juce::Point<int>(actualX[0], 0),
			juce::Point<int>(actualX[1] == getWidth()-5 ? actualX[1]+5 : actualX[1],
				getHeight()
				)));

		g.setOpacity(0.7f);
		g.fillEllipse(grabArea[0].toFloat());
		g.fillEllipse(grabArea[1].toFloat());
}
void RangeHandle::resized()
{
	auto width = getWidth();
	auto height = getHeight();

	grabArea[0].setBounds((width * contexts[0].relativeX)-5, height * 0.02f, 10, 10);
	grabArea[1].setBounds(((width-5) * contexts[1].relativeX)-5, height * 0.02f, 10, 10);
}
void RangeHandle::mouseDown(const juce::MouseEvent& event)
{
	if (grabArea[0].contains(event.getMouseDownPosition()))
	{
		DBG("Grabbed Start!");
		grabbed = RangeHandleSide::Start;
		startAtt.beginGesture();
	}
	else if (grabArea[1].contains(event.getMouseDownPosition()))
	{
		DBG("Grabbed End!");
		grabbed = RangeHandleSide::End;
		endAtt.beginGesture();
	}
	else
	{
		grabbed = RangeHandleSide::None;
	}
}
void RangeHandle::mouseDrag(const juce::MouseEvent& event)
{
	if (grabbed == RangeHandleSide::None) return;

	int startX = grabArea[0].getX();
	int endX = grabArea[1].getX();

	int newX = event.getPosition().getX();
	if (grabbed == RangeHandleSide::Start)
	{
		if (newX >= endX) newX = endX - 1;
		if (getMouseXYRelative().getX() < 0) newX = 0;
		contexts[0].relativeX = ((float)newX / (float)getWidth());
		grabArea[0] = grabArea[0].withX(newX);
		startAtt.setValueAsPartOfGesture(fileBuffer->buffer.getNumSamples() * contexts[0].relativeX);
	}
	else if (grabbed == RangeHandleSide::End)
	{
		if (newX <= startX) newX = startX + 1;
		if (getMouseXYRelative().getX() > getWidth()) newX = getWidth() - 5;
		contexts[1].relativeX = ((float)newX / (float)getWidth());
		grabArea[1] = grabArea[1].withX(newX);
		endAtt.setValueAsPartOfGesture(fileBuffer->buffer.getNumSamples() * contexts[1].relativeX);
	}
	resized();
	repaint();
	
}
void RangeHandle::mouseUp(const juce::MouseEvent& event)
{
	if (grabbed == RangeHandleSide::Start)
	{
		startAtt.endGesture();
	}
	else if (grabbed == RangeHandleSide::End)
	{
		endAtt.endGesture();
	}
	grabbed = RangeHandleSide::None;
	return;
}
FileView::FileView(SharedFileBuffer* buf, juce::AudioProcessorValueTreeState& vts) : handle(vts, buf)
{
	fileBuffer = buf;
	addAndMakeVisible(handle);
}
void FileView::drawFile()
{
	//TODO: Change maybe to draw path? might be easier on the CPU rather than rescaling the image(also maybe resolution)?
	auto imageBounds = fileImage.getBounds();
	int width = imageBounds.getWidth();
	int height = imageBounds.getHeight();

	fileImage.clear(imageBounds, juce::Colours::transparentBlack);
	juce::Graphics drawingImage(fileImage);
	drawingImage.setColour(juce::Colours::white);

	int numSamples = fileBuffer->buffer.getNumSamples();

	const int midPointY = imageBounds.getCentreY();


	float x = 0.f;
	float stride = (float)width / (numSamples / (float)samplesPerWindow);
	const int maxLineHeight = imageBounds.getY() + midPointY;


	float sampleAvg = 0.0f;
	const float* buffer = fileBuffer->buffer.getReadPointer(0);
	for (int i = 0; i < numSamples; ++i)
	{
		if (!(i % samplesPerWindow))
		{
			float lineHeight = (sampleAvg / (float)samplesPerWindow) * maxLineHeight;
			float rectY = abs(midPointY - (lineHeight * 0.5f));
			drawingImage.fillRect(x, rectY, 0.2, lineHeight);
			sampleAvg = 0.0f;
			x += stride;
		}
		sampleAvg += abs(buffer[i]);
	}
}
void FileView::paint(juce::Graphics &g)
{
	g.drawImage(fileImage, getBounds().toFloat());
}
void FileView::resized()
{
	//TODO: Fix constant drawing.
	auto bounds = getLocalBounds();
	handle.setBounds(bounds);
}

void FileView::mouseDown(const juce::MouseEvent& event)
{
	juce::Point<int> point = event.getMouseDownPosition();
	if (handle.grabArea[0].contains(point))
	{
		grabbed = RangeHandleSide::Start;
	}
	else if (handle.grabArea[1].contains(point))
	{
		grabbed = RangeHandleSide::End;
	}
	
}
void FileView::mouseDrag(const juce::MouseEvent& event)
{
#if 0
	if (grabbed == RangeHandleSide::None) return;

	int mouseX;
	int newX = event.getPosition().getX();
	if (grabbed == RangeHandleSide::Start)
	{
		if (newX >= handle.x[1]) newX = handle.x[1] - 1;
		if (getMouseXYRelative().getX() < 0) newX = 0;
		handle.x[0] = newX;
		handle.grabArea[0].setX(handle.x[0]);
		mouseX = handle.x[0];
	}
	else if (grabbed == RangeHandleSide::End)
	{
		if (newX <= handle.x[0]) newX = handle.x[0] + 1;
		if (getMouseXYRelative().getX() > getWidth()) newX = getWidth() - 5;
		handle.x[1] = newX;
		handle.grabArea[1].setX(handle.x[1]);
		mouseX = handle.x[1];
	}

	int sample = mapPositionToSample(mouseX);

	sendActionMessage(juce::String(grabbed == RangeHandleSide::Start ? "start=" : "end=")+juce::String(sample));
	repaint();
	//DBG("Number of Samples: " + juce::String(numSamples) + "\nSelected Sample: " + juce::String(sample));
#endif
}
void FileView::mouseUp(const juce::MouseEvent& event)
{
	grabbed = RangeHandleSide::None;
	return;
}
int FileView::mapPositionToSample(int x)
{
	const int width = getWidth();
	const int numSamples = fileBuffer->buffer.getNumSamples();
	const float stride = (float)width / (numSamples / (float)samplesPerWindow);
	const float countStride = x / stride;

	return ((int)countStride * samplesPerWindow) + (int)((countStride - (int)countStride) * samplesPerWindow);
}
void FileView::actionListenerCallback(const juce::String& message)
{
#if 0
	sendActionMessage(juce::String("start=")+juce::String(mapPositionToSample(handle.x[0])));
	sendActionMessage(juce::String("end=")+juce::String(mapPositionToSample(handle.x[1])));
#endif
}
