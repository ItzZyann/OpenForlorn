#pragma once
#include "../FL_Block.h"
class ShakyBlock : public FL_Block {
public: static ShakyBlock* create(const Data& data,float delay);bool initShaky(const Data&,float delay);void activateObject(bool deactivating);void resetObject();
private: ShakyBlock():delay_(0),spawn_(CCPointZero){}void fall();float delay_;CCPoint spawn_;
};
