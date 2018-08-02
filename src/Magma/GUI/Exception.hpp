#pragma once

#include <stdexcept>

namespace Magma
{
	namespace GUI
	{
		/// <summary>
		///		Thrown when there is a GUI element related error
		/// </summary>
		class ElementError : public std::runtime_error
		{
		public:
			using std::runtime_error::runtime_error;
		};
	}
}