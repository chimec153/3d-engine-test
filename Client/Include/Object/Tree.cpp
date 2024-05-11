#include "Tree.h"

Client::Tree::Tree()
{
}

bool Client::Tree::Init()
{
    if (!__super::Init())
    {
        return false;
    }

    Load(TEXT("SmallCampingBundle\\Tree\\Tree.obj"));

    return true;
}
