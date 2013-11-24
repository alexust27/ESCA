#ifndef VARIABLE_H
#define VARIABLE_H

#include <string>

enum EMetaType
{
	VAR_COMMON,
	VAR_POINTER,
	VAR_ARRAY_POINTER
};

class Variable
{
	public:
		Variable() {}
		Variable(const Variable &v) { name = v.name; type = v.type; metaType = v.metaType; }
		Variable (const std::string &t, const std::string &n, EMetaType mt) : name(n), type(t) { metaType = mt;}

		inline std::string Name() const { return name; }
		inline std::string Type() const { return type; }
		inline EMetaType MetaType() const { return metaType; }
		inline void MetaType(EMetaType mt) { metaType = mt; }

		std::string TypeSMT() const;
	protected:
		std::string name;
		std::string type;
		EMetaType metaType;
};

#endif
