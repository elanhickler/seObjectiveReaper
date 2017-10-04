/*
  ==============================================================================

    salt_exceptions.h
    Created: 6 Sep 2016 1:10:47pm
    Author:  jim

  ==============================================================================
*/

#ifndef SALT_EXCEPTIONS_H_INCLUDED
#define SALT_EXCEPTIONS_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include <exception>


class SaltException
	:
	public std::runtime_error
{
public:
	SaltException(const String& exceptionMessage) 
		: 
		std::runtime_error(exceptionMessage.toStdString()) {}
};



#endif  // SALT_EXCEPTIONS_H_INCLUDED
