/**
 * @file audioengine_openal.cpp
 * @brief implementation of audio engine using OpenAL
 * support as a OpenAL 3D implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llaudioengine.h"

#include "lllistener_openal.h"

LLListener_OpenAL::LLListener_OpenAL()
    : LLListener(),
      mRolloffFactor(1.f)
{
}

void LLListener_OpenAL::translate(const LLVector3& offset)
{
    //LL_INFOS() << "LLListener_OpenAL::translate() : " << offset << LL_ENDL;
    LLListener::translate(offset);
}

void LLListener_OpenAL::setPosition(const LLVector3& pos)
{
    //LL_INFOS() << "LLListener_OpenAL::setPosition() : " << pos << LL_ENDL;
    LLListener::setPosition(pos);
}

void LLListener_OpenAL::setVelocity(const LLVector3& vel)
{
    LLListener::setVelocity(vel);
}

void LLListener_OpenAL::orient(const LLVector3& up, const LLVector3& at)
{
    //LL_INFOS() << "LLListener_OpenAL::orient() up: " << up << " at: " << at << LL_ENDL;
    LLListener::orient(up, at);
}

void LLListener_OpenAL::commitDeferredChanges()
{
    ALfloat orientation[] = {
        mListenAt.mV[0],
        mListenAt.mV[1],
        mListenAt.mV[2],
        mListenUp.mV[0],
        mListenUp.mV[1],
        mListenUp.mV[2],
    };

    ALfloat velocity[3] = {
        mVelocity.mV[0],
        mVelocity.mV[1],
        mVelocity.mV[2],
    };

    alListenerfv(AL_ORIENTATION, orientation);
    alListenerfv(AL_POSITION, mPosition.mV);
    alListenerfv(AL_VELOCITY, velocity);
}

void LLListener_OpenAL::setDopplerFactor(F32 factor)
{
    //LL_INFOS() << "LLListener_OpenAL::setDopplerFactor() : " << factor << LL_ENDL;
    alDopplerFactor(factor);
}

F32 LLListener_OpenAL::getDopplerFactor()
{
    ALfloat factor;
    factor = alGetFloat(AL_DOPPLER_FACTOR);
    //LL_INFOS() << "LLListener_OpenAL::getDopplerFactor() : " << factor << LL_ENDL;
    return factor;
}


void LLListener_OpenAL::setRolloffFactor(F32 factor)
{
    mRolloffFactor = factor;
}

F32 LLListener_OpenAL::getRolloffFactor()
{
    return mRolloffFactor;
}


