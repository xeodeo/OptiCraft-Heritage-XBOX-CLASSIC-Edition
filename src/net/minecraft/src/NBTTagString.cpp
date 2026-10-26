#include "NBTTagString.h"

#include <istream>
#include <ostream>

#include "java/IOUtil.h"

NBTTagString::NBTTagString()
{
}

NBTTagString::NBTTagString(const jstring &s)
{
	data = s;
}

NBTTagString::NBTTagString(const jstring &name, const jstring &s)
	: NBTBase(name)
{
	data = s;
}

void NBTTagString::writeTagContents(std::ostream &dataoutput)
{
	IOUtil::writeUTF(dataoutput, data);
}

void NBTTagString::readTagContents(std::istream &datainput)
{
	data = IOUtil::readUTF(datainput);
}

byte_t NBTTagString::getType()
{
	return 8;
}

NBTBase *NBTTagString::copy()
{
	NBTTagString *tag = new NBTTagString(data);
	tag->setKey(getKey());
	return tag;
}

jstring NBTTagString::toString()
{
	return data;
}

bool NBTTagString::equals(NBTBase *other)
{
	if (!NBTBase::equals(other)) return false;
	auto *tag = dynamic_cast<NBTTagString *>(other);
	return tag != nullptr && data == tag->data;
}

int_t NBTTagString::hashCode()
{
	return NBTBase::hashCode() ^ String::hashCode(data);
}
