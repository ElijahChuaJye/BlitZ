/*****************************************************************//**
 * \file   TradingApplication.cpp
 * \brief  Trading Application implementations
 * 
 * \author elija
 * \date   September 2026
 *********************************************************************/
#include "TradingApplication.h"



void TradingApplication::onFrameStart(std::uint32_t frameIndex){
	(void)frameIndex;

	if (m_queueTestRun) {
		return;
	}

	m_queueTestRun = true;

	//Creating the queue
	SPSCQueue<L2OrderUpdate, 8> queue{};

	L2OrderUpdate sent{};	//To send the sample tick into the queue. Push(sent)

	sent.price = 123.45;
	sent.quantity = 10.0;
	sent.timestamp = 1;
	sent.orderCount = 1;
	sent.side = OrderSide::buy;

	const bool pushed = queue.push(sent);
	std::cout << "Push suceeeded: " << pushed << '\n';

	L2OrderUpdate received{};	//Where pop copies a tick from the queue Pop(received)

	const bool popped = queue.pop(received);
	std::cout << "Pop succeeded: " << popped << '\n';

	if (popped) {
		std::cout << "Received price: " << received.price << '\n';
	}
}
