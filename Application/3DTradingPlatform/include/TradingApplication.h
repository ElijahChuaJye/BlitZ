/*****************************************************************//**
 * \file   TradingApplication.h
 * \brief  Trading-platform specialization of the reusable Vulkan application
 * 
 * \author elija
 * \date   September 2026
 *********************************************************************/

#ifndef TRADING_APPLICATION_H
#define TRADING_APPLICATION_H

#include <cstdint>

#include "core/Application.h"

/**
 * @brief Connects the trading-domain processing to generic Engine frame loop.
 * 
 * Market feeds, order books and tick processing belogn here
 * 
 */

class TradingApplication final : public Application {		//final primarily prevents further inheritance. It may permit devirtualization, but a performance improvement is not guaranteed.
protected:
	/**
	 * @brief Performs trading-specific work at the beginning of a safe frame.
	 *
	 * The market-data queue will be drained here in a later step. For now, the
	 * empty implementation verifies the application-extension boundary.
	 */
	void onFrameStart(std::uint32_t frameIndex) override;
};

#endif	//TRADING_APPLICATION_H



