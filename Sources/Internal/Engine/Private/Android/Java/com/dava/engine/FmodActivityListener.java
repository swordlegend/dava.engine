package com.dava.engine;

import org.fmod.FMODAudioDevice;

public class FmodActivityListener extends DavaActivity.ActivityListenerImpl
{
	private FMODAudioDevice fmodDevice;
	
	public FmodActivityListener()
	{
		final FmodActivityListener instance = this;
		DavaActivity.instance().runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				fmodDevice = new FMODAudioDevice();
				
				DavaActivity activity = DavaActivity.instance();
				
				activity.registerActivityListener(instance);
				
				// Handle a case when FmodActivityListener is being created while activity is already visible
				// We need to start fmod device in this case since onStart won't be called
				if (activity.isVisible())
				{
					fmodDevice.start();
				}
			}
		});
	}
	
	public void unregister()
	{
		DavaActivity activity = DavaActivity.instance();
		if (activity != null)
		{
			activity.unregisterActivityListener(this);
		}
	}
	
	@Override
	public void onStart()
	{	
		fmodDevice.start();
	}
	
	@Override
	public void onStop()
	{
		fmodDevice.stop();
	}
}