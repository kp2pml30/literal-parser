/* Copyright (C) 2020 kp2pml30
 * MIT license
 */
#pragma once

#include <iostream>
#include <tuple>
#include <type_traits>

class LiteralParser
{
private:
	template<typename Type, Type cha>
	static constexpr bool IsWS(void)
	{
		return cha == ' ' || cha == '\n' || cha == '\t';
	}

	/*** constant parser ***/
	template<std::size_t ex>
	class Mul10
	{
	public:
		static constexpr int Go(void)
		{
			if constexpr (ex == 0)
				return 1;
			else
				return 10 * Mul10<ex - 1>::Go();
		}
	};
	template<typename Type, Type...str>
	class ScanConst;
	template<typename Type>
	class ScanConst<Type> { public: static constexpr std::tuple<std::size_t, int> Go(void) { return {0, 0}; } };
	template<typename Type, Type first, Type ...tail>
	class ScanConst<Type, first, tail...>
	{
	public:
		static constexpr std::tuple<std::size_t, int> Go(void)
		{
			if constexpr (first >= '0' && first <= '9')
			{
				constexpr auto tup = ScanConst<Type, tail...>::Go();
				constexpr std::size_t off = std::get<0>(tup);
				constexpr uint num = std::get<1>(tup);
				return {1 + off, (first - '0') * Mul10<off>::Go() + num};
			}
			else
				return {0, 0};
		}
	};

	/*** operator parser ***/
	enum class BinaryOperatorType
	{
		OR,
		XOR,
		AND,
		PLUS,
		MINUS,
		MULTIPLY,
		DIVIDE,

		ERROR
	};
	template<BinaryOperatorType type>
	static constexpr int OperatorPriority(void)
	{
		if constexpr (type == BinaryOperatorType::OR)
			return 0;
		else if constexpr (type == BinaryOperatorType::XOR)
			return 1;
		else if constexpr (type == BinaryOperatorType::AND)
			return 2;
		else if constexpr (type >= BinaryOperatorType::PLUS && type <= BinaryOperatorType::MINUS)
			return 3;
		else if constexpr (type >= BinaryOperatorType::MULTIPLY && type <= BinaryOperatorType::DIVIDE)
			return 4;
		else if constexpr (type == BinaryOperatorType::ERROR)
			return 5;
		else
			static_assert(type <= BinaryOperatorType::ERROR, "Kernel error : no operator in OperatorPriority");
	}
	static constexpr int GetOperatorMaxPriority(void)
	{
		return OperatorPriority<BinaryOperatorType::ERROR>();
	}
	template<BinaryOperatorType type, int left, int right>
	static constexpr int ApplyOperator(void)
	{
		if constexpr (type == BinaryOperatorType::OR)
			return left | right;
		else if constexpr (type == BinaryOperatorType::XOR)
			return left ^ right;
		else if constexpr (type == BinaryOperatorType::AND)
			return left & right;
		else if constexpr (type == BinaryOperatorType::PLUS)
			return left + right;
		else if constexpr (type == BinaryOperatorType::MINUS)
			return left - right;
		else if constexpr (type == BinaryOperatorType::MULTIPLY)
			return left * right;
		else if constexpr (type == BinaryOperatorType::DIVIDE)
			return left / right;
		else
			static_assert(type <= BinaryOperatorType::DIVIDE, "Kernel error : no operator in ApplyOperator");
	}

	template<typename RetType, template<typename Type1, Type1 ...args> typename Func, typename Type, Type ...args>
	class SkipWS;
	template<typename RetType, template<typename Type1, Type1 ...args> typename Func, typename Type, Type first, Type ...args>
	class SkipWS<RetType, Func, Type, first, args...>
	{
	public:
		static constexpr RetType Go(void)
		{
			if constexpr (IsWS<Type, first>())
			{
				constexpr auto ret = Func<Type, args...>::Go();
				return {1 + std::get<0>(ret), std::get<1>(ret)};
			}
			else
				return Func<Type, first, args...>::Go();
		}
	};
	template<typename RetType, template<typename Type1, Type1 ...args> typename Func, typename Type>
	class SkipWS<RetType, Func, Type>
	{
	public:
		static constexpr RetType Go(void)
		{
			return Func<Type>::Go();
		}
	};
	template<typename Type, Type...str>
	class ScanBinary;
	template<typename Type>
	class ScanBinary<Type> { public: static constexpr std::tuple<std::size_t, BinaryOperatorType> Go(void) { return {0, BinaryOperatorType::PLUS}; } };
	template<typename Type, Type first, Type ...tail>
	class ScanBinary<Type, first, tail...>
	{
	public:
		static constexpr std::tuple<std::size_t, BinaryOperatorType> Go(void)
		{
			switch (first)
			{
			case '|':
				return {1, BinaryOperatorType::OR};
			case '^':
				return {1, BinaryOperatorType::XOR};
			case '&':
				return {1, BinaryOperatorType::AND};
			case '+':
				return {1, BinaryOperatorType::PLUS};
			case '-':
				return {1, BinaryOperatorType::MINUS};
			case '*':
				return {1, BinaryOperatorType::MULTIPLY};
			case '/':
				return {1, BinaryOperatorType::DIVIDE};
			default:
				return {0, BinaryOperatorType::ERROR};
			}
		}
	};
	/*** Recursive descent parser ***/
	template<uint shift, typename Type, Type ...args>
	class GetShifted
	{
	public:
		static constexpr char Go(void) { return 0; }
	};
	template<uint shift, typename Type, Type first, Type ...args>
	class GetShifted<shift, Type, first, args...>
	{
	public:
		static constexpr char Go(void)
		{
			if constexpr (shift == 0)
				return first;
			else
				return GetShifted<shift - 1, Type, args...>::Go();
		}
	};

	template<typename Type, Type...str>
	class ScanValue;
	template<typename Type>
	class ScanValue<Type> { public: static constexpr std::tuple<std::size_t, int> Go(void) { return {0, 0}; } };
	template<typename Type, Type first, Type ...tail>
	class ScanValue<Type, first, tail...>
	{
	public:
		static constexpr std::tuple<std::size_t, int> Go(void)
		{
			if constexpr (first == '(')
			{
				constexpr auto ret = RecParse<0, Type, tail...>::Go();
				static_assert(GetShifted<std::get<0>(ret), Type, tail...>::Go() == ')', "')' expected");
				return {std::get<0>(ret) + 2, std::get<1>(ret)};
			}
			else if constexpr (first == '-')
			{
				constexpr auto ret = RecParse<0, Type, tail...>::Go();
				return {1 + std::get<0>(ret), -std::get<1>(ret)};
			}
			else if constexpr (first == '~')
			{
				constexpr auto ret = RecParse<0, Type, tail...>::Go();
				return {1 + std::get<0>(ret), ~std::get<1>(ret)};
			}
			else if constexpr (first == '+')
			{
				constexpr auto ret = RecParse<0, Type, tail...>::Go();
				return {1 + std::get<0>(ret), std::get<1>(ret)};
			}
			return ScanConst<Type, first, tail...>::Go();
		}
	};
	template<uint depth, typename Type, Type ...tail>
	class RecParse
	{
	public:
		template<int left>
		static constexpr std::tuple<std::size_t, int> GoL()
		{
			static_assert(depth != GetOperatorMaxPriority(), "Kernel error : unexpected GoL");
			constexpr auto op = SkipWS<std::tuple<uint, BinaryOperatorType>, ScanBinary, Type, tail...>::Go();
			if constexpr (std::get<0>(op) == 0 || std::get<1>(op) == BinaryOperatorType::ERROR || OperatorPriority<std::get<1>(op)>() != depth)
				return {0, left};
			else
			{
				constexpr auto right = SkipRec<std::get<0>(op), depth + 1, Type, tail...>::Go();
				static_assert(std::get<0>(right) != 0 || CheckEnd<std::get<0>(op), Type, tail...>::Go(), "Unexpected end of expression. value expected.");
				constexpr uint currentoff = std::get<0>(op) + std::get<0>(right);
				constexpr auto ret = SkipRec<currentoff, depth, Type, tail...>::template GoL<ApplyOperator<std::get<1>(op), left, std::get<1>(right)>()>();
				return {currentoff + std::get<0>(ret), std::get<1>(ret)};
			}
		}
		static constexpr std::tuple<std::size_t, int> Go(void)
		{
			if constexpr (depth == GetOperatorMaxPriority())
			{
				constexpr auto ret = SkipWS<std::tuple<uint, int>, ScanValue, Type, tail...>::Go();
				static_assert(std::get<0>(ret) != 0, "Constant expected");
				return ret;
			}
			else
			{
				constexpr auto left = RecParse<depth + 1, Type, tail...>::Go();
				static_assert(std::get<0>(left) != 0, "Unexpected end of expression");
				constexpr auto ret = SkipRec<std::get<0>(left), depth, Type, tail...>::template GoL<std::get<1>(left)>();
				return {std::get<0>(left) + std::get<0>(ret), std::get<1>(ret)};
			}
		}
	};
	template<uint depth, typename Type>
	class RecParse<depth, Type>
	{
	public:
		template<int left>
		static constexpr std::tuple<std::size_t, int> GoL(void)
		{
			return {0, left};
		}
		static constexpr std::tuple<std::size_t, int> Go(void)
		{
			return {0, 0};
		}
	};
	template<uint skip, uint depth, typename Type, Type ...tail>
	class SkipRec;
	template<uint skip, uint depth, typename Type, Type first, Type ...tail>
	class SkipRec<skip, depth, Type, first, tail...>
	{
	public:
		static constexpr std::tuple<std::size_t, int> Go(void)
		{
			if constexpr (skip == 0)
				return RecParse<depth, Type, first, tail...>::Go();
			else
				return SkipRec<skip - 1, depth, Type, tail...>::Go();
		}
		template<int left>
		static constexpr std::tuple<std::size_t, int> GoL(void)
		{
			if constexpr (skip == 0)
				return RecParse<depth, Type, first, tail...>::template GoL<left>();
			else
				return SkipRec<skip - 1, depth, Type, tail...>::template GoL<left>();
		}
	};
	template<uint depth, typename Type>
	class SkipRec<0, depth, Type>
	{
	public:
		static constexpr std::tuple<std::size_t, int> Go(void)
		{
			return RecParse<depth, Type>::Go();
		}
		template<int left>
		static constexpr std::tuple<std::size_t, int> GoL(void)
		{
			return RecParse<depth, Type>::template GoL<left>();
		}
	};

	// check spaces in the end of the expr
	template<uint skip, typename Type, Type ...args>
	class CheckEnd;
	template<uint skip, typename Type, Type first, Type ...args>
	class CheckEnd<skip, Type, first, args...>
	{
	public:
		static constexpr bool Go(void)
		{
			return CheckEnd<skip - 1, Type, args...>::Go();
		}
	};
	template<typename Type>
	class CheckEnd<0, Type> { public: static constexpr bool Go(void) { return true; } };
	template<typename Type, Type first, Type ...tail>
	class CheckEnd<0, Type, first, tail...>
	{
	public:
		static constexpr bool Go(void)
		{
			if constexpr(IsWS<Type, first>())
				return CheckEnd<0, Type, tail...>::Go();
			else
				return false;
		}
	};
public:
	// can't scan Integer.MIN_VALUE
	template<typename Type, Type...str>
	static constexpr int Evaluate(void)
	{
		constexpr auto pex = RecParse<0, Type, str...>::Go();
		static_assert(std::get<0>(pex) == sizeof...(str) || CheckEnd<std::get<0>(pex), Type, str...>::Go(), "Parsing not finished string");
		return std::get<1>(pex);
	}
};

template<typename Type, Type...str>
constexpr int operator""_calc(void)
{
	return LiteralParser::Evaluate<Type, str...>();
}

// End of 'LiteralParser.hpp' file
