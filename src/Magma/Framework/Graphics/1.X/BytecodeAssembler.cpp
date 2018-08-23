#include "BytecodeAssembler.hpp"

#include "../../String/Conversion.hpp"
#include "../../Memory/Endianness.hpp"

#include <sstream>
#include <map>

using namespace Magma::Framework::Graphics_V1X;
using namespace Magma::Framework;

std::map<std::string, BytecodeOpCode> instructionSet =
{
	std::make_pair("decli1", BytecodeOpCode::DeclI1),
	std::make_pair("decli2", BytecodeOpCode::DeclI2),
	std::make_pair("decli3", BytecodeOpCode::DeclI3),
	std::make_pair("decli4", BytecodeOpCode::DeclI4),
	std::make_pair("decli22", BytecodeOpCode::DeclI22),
	std::make_pair("decli33", BytecodeOpCode::DeclI33),
	std::make_pair("decli44", BytecodeOpCode::DeclI44),

	std::make_pair("declf1", BytecodeOpCode::DeclF1),
	std::make_pair("declf2", BytecodeOpCode::DeclF2),
	std::make_pair("declf3", BytecodeOpCode::DeclF3),
	std::make_pair("declf4", BytecodeOpCode::DeclF4),
	std::make_pair("declf22", BytecodeOpCode::DeclF22),
	std::make_pair("declf33", BytecodeOpCode::DeclF33),
	std::make_pair("declf44", BytecodeOpCode::DeclF44),

	std::make_pair("declb", BytecodeOpCode::DeclB),

	std::make_pair("varin0", BytecodeOpCode::VarIn0),
	std::make_pair("varin1", BytecodeOpCode::VarIn1),
	std::make_pair("varout", BytecodeOpCode::VarOut),

	std::make_pair("assign", BytecodeOpCode::Assign),
	std::make_pair("add", BytecodeOpCode::Add),
	std::make_pair("sub", BytecodeOpCode::Sub),
	std::make_pair("mul", BytecodeOpCode::Mul),
	std::make_pair("div", BytecodeOpCode::Div),
	std::make_pair("mod", BytecodeOpCode::Mod),
	std::make_pair("and", BytecodeOpCode::And),
	std::make_pair("or", BytecodeOpCode::Or),
	std::make_pair("not", BytecodeOpCode::Not),
	std::make_pair("greater", BytecodeOpCode::Greater),
	std::make_pair("less", BytecodeOpCode::Less),
	std::make_pair("gequal", BytecodeOpCode::GEqual),
	std::make_pair("lequal", BytecodeOpCode::LEqual),
	std::make_pair("equal", BytecodeOpCode::Equal),
	std::make_pair("nequal", BytecodeOpCode::NEqual),

	std::make_pair("seti1", BytecodeOpCode::SetI1),
	std::make_pair("seti2", BytecodeOpCode::SetI2),
	std::make_pair("seti3", BytecodeOpCode::SetI3),
	std::make_pair("seti4", BytecodeOpCode::SetI4),
	std::make_pair("setf1", BytecodeOpCode::SetF1),
	std::make_pair("setf2", BytecodeOpCode::SetF2),
	std::make_pair("setf3", BytecodeOpCode::SetF3),
	std::make_pair("setf4", BytecodeOpCode::SetF4),

	std::make_pair("asf2cmp", BytecodeOpCode::AsF2Cmp),
	std::make_pair("asf3cmp", BytecodeOpCode::AsF3Cmp),
	std::make_pair("asf4cmp", BytecodeOpCode::AsF4Cmp),
	std::make_pair("asi2cmp", BytecodeOpCode::AsI2Cmp),
	std::make_pair("asi3cmp", BytecodeOpCode::AsI3Cmp),
	std::make_pair("asi4cmp", BytecodeOpCode::AsI4Cmp),

	std::make_pair("gef2cmp", BytecodeOpCode::GeF2Cmp),
	std::make_pair("gef3cmp", BytecodeOpCode::GeF3Cmp),
	std::make_pair("gef4cmp", BytecodeOpCode::GeF4Cmp),
	std::make_pair("gei2cmp", BytecodeOpCode::GeI2Cmp),
	std::make_pair("gei3cmp", BytecodeOpCode::GeI3Cmp),
	std::make_pair("gei4cmp", BytecodeOpCode::GeI4Cmp),

	std::make_pair("opscope", BytecodeOpCode::OpScope),
	std::make_pair("clscope", BytecodeOpCode::ClScope),
	std::make_pair("discard", BytecodeOpCode::Discard),
	std::make_pair("return", BytecodeOpCode::Return),
	std::make_pair("while", BytecodeOpCode::While),
	std::make_pair("if", BytecodeOpCode::If),
	std::make_pair("else", BytecodeOpCode::Else),

	std::make_pair("mulmat", BytecodeOpCode::MulMat),
	std::make_pair("smple2d", BytecodeOpCode::Smple2D),
	std::make_pair("cos", BytecodeOpCode::Cos),
	std::make_pair("sin", BytecodeOpCode::Sin),
	std::make_pair("tan", BytecodeOpCode::Tan),
	std::make_pair("acos", BytecodeOpCode::ACos),
	std::make_pair("asin", BytecodeOpCode::ASin),
	std::make_pair("atan", BytecodeOpCode::ATan),
	std::make_pair("degrees", BytecodeOpCode::Degrees),
	std::make_pair("radians", BytecodeOpCode::Radians),
	std::make_pair("pow", BytecodeOpCode::Pow),
	std::make_pair("exp", BytecodeOpCode::Exp),
	std::make_pair("log", BytecodeOpCode::Log),
	std::make_pair("exp2", BytecodeOpCode::Exp2),
	std::make_pair("log2", BytecodeOpCode::Log2),
	std::make_pair("sqrt", BytecodeOpCode::Sqrt),
	std::make_pair("isqrt", BytecodeOpCode::ISqrt),
	std::make_pair("abs", BytecodeOpCode::Abs),
	std::make_pair("sign", BytecodeOpCode::Sign),
	std::make_pair("floor", BytecodeOpCode::Floor),
	std::make_pair("ceil", BytecodeOpCode::Ceil),
	std::make_pair("fract", BytecodeOpCode::Fract),
	std::make_pair("lerp", BytecodeOpCode::Lerp),
	std::make_pair("clamp", BytecodeOpCode::Clamp),
	std::make_pair("dot", BytecodeOpCode::Dot),
	std::make_pair("cross", BytecodeOpCode::Cross),
	std::make_pair("nrmlize", BytecodeOpCode::Nrmlize),
	std::make_pair("round", BytecodeOpCode::Round),
	std::make_pair("trnspse", BytecodeOpCode::Trnspse),
	std::make_pair("reflect", BytecodeOpCode::Reflect),
	std::make_pair("min", BytecodeOpCode::Min),
	std::make_pair("max", BytecodeOpCode::Max),
};

BytecodeOpCode GetOpCode(const std::string& str)
{
	std::string lowerCase = str;
	String::ToLower(lowerCase);
	auto it = instructionSet.find(lowerCase);
	if (it == instructionSet.end())
	{
		std::stringstream ss;
		ss << "Failed to assemble binary bytecode from bytecode string:" << std::endl;
		ss << "Unsupported op code \"" << str << "\"";
		throw ShaderError(ss.str());
	}
	else
		return it->second;
}

size_t Magma::Framework::Graphics_V1X::BytecodeAssembler::Assemble(const std::string & code, char * out, size_t maxSize)
{
	std::stringstream ss(code);

	size_t size = 0;
	std::string line;
	while (std::getline(ss, line))
	{
		bool gotOp = false;
		std::string name = "";

		for (const auto& c : line)
		{
			if (c == ' ' || c == '\t')
				continue;
			if (c == '#')
				break;
			if (c == ',')
			{
				if (name.empty())
					continue;
				if (!gotOp)
				{
					auto op = GetOpCode(name);
					((unsigned char*)out)[size] = (unsigned char)op;
					gotOp = true;

					++size;
					if (size > maxSize)
					{
						std::stringstream ss;
						ss << "Failed to assemble binary bytecode from bytecode string:" << std::endl;
						ss << "Not enough memory to assembly binary bytecode:" << std::endl;
						ss << "Stopped at line \"" << line << "\"";
						throw ShaderError(ss.str());
					}
				}
				else
				{
					if (name[0] == 'x' && name.size() > 2 && name[1] == 'i') // Hexadecimal int
					{
						if (size + 4 > maxSize)
						{
							std::stringstream ss;
							ss << "Failed to assemble binary bytecode from bytecode string:" << std::endl;
							ss << "Not enough memory to assembly binary bytecode:" << std::endl;
							ss << "Stopped at line \"" << line << "\"";
							throw ShaderError(ss.str());
						}

						auto value = std::stol(name.substr(2), 0, 16);
						Memory::ToBigEndian4(&value, &value);
						memcpy(out + size, &value, 4);
						size += 4;
					}
					else if (name[0] == 'x' && name.size() > 2 && name[1] == 'b') // Hexadecimal byte
					{
						if (size + 1 > maxSize)
						{
							std::stringstream ss;
							ss << "Failed to assemble binary bytecode from bytecode string:" << std::endl;
							ss << "Not enough memory to assembly binary bytecode:" << std::endl;
							ss << "Stopped at line \"" << line << "\"";
							throw ShaderError(ss.str());
						}

						out[size] = std::stol(name.substr(2), 0, 16);
						size++;
					}
					else if(name[0] == 'd' && name.size() > 2 && name[1] == 'i') // Decimal int
					{
						if (size + 4 > maxSize)
						{
							std::stringstream ss;
							ss << "Failed to assemble binary bytecode from bytecode string:" << std::endl;
							ss << "Not enough memory to assembly binary bytecode:" << std::endl;
							ss << "Stopped at line \"" << line << "\"";
							throw ShaderError(ss.str());
						}

						auto value = std::stol(name.substr(2), 0, 10);
						Memory::ToBigEndian4(&value, &value);
						memcpy(out + size, &value, 4);
						size += 4;
					}
					else if (name[0] == 'd' && name.size() > 2 && name[1] == 'b') // Decimal byte
					{
						if (size + 1 > maxSize)
						{
							std::stringstream ss;
							ss << "Failed to assemble binary bytecode from bytecode string:" << std::endl;
							ss << "Not enough memory to assembly binary bytecode:" << std::endl;
							ss << "Stopped at line \"" << line << "\"";
							throw ShaderError(ss.str());
						}

						out[size] = std::stol(name.substr(2), 0, 10);
						size++;
					}
					else if (name[0] == 'f' && name.size() > 1) // Float
					{
						if (size + 4 > maxSize)
						{
							std::stringstream ss;
							ss << "Failed to assemble binary bytecode from bytecode string:" << std::endl;
							ss << "Not enough memory to assembly binary bytecode:" << std::endl;
							ss << "Stopped at line \"" << line << "\"";
							throw ShaderError(ss.str());
						}

					
						mfmF32 value = std::stof(name.substr(1));
						Memory::ToBigEndian4(&value, &value);
						memcpy(out + size, &value, 4);
						size += 4;
					}
					else
					{
						std::stringstream ss;
						ss << "Failed to assemble binary bytecode from bytecode string:" << std::endl;
						ss << "Invalid param:" << std::endl;
						ss << "Stopped at line \"" << line << "\"";
						throw ShaderError(ss.str());
					}

				}

				name = "";
				continue;
			}
			
			name += c;
		}
	}

	return size;
}