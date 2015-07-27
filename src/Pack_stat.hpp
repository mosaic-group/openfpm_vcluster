/*
 * Pack_stat.hpp
 *
 *  Created on: Jul 17, 2015
 *      Author: i-bird
 */

#ifndef SRC_PACK_STAT_HPP_
#define SRC_PACK_STAT_HPP_

/*! \brief Packing statistics
 *
 *
 */
class Pack_stat
{
	size_t p_mark = 0;
	size_t un_ele = 0;

public:

	Pack_stat()
	:p_mark(0),un_ele(0)
	{}

	/*! \brief Increment the request pointer
	 *
	 *
	 */
	inline void incReq()
	{
		un_ele++;
	}

	/*! \brief return the actual request for packing
	 *
	 * \return the actual request for packing
	 *
	 */
	inline size_t reqPack()
	{
		return un_ele;
	}

	/*! \brief Mark
	 *
	 *
	 *
	 */
	inline void mark()
	{
		p_mark = un_ele;
	}

	/*! \brief Return the mark
	 *
	 * \return the mark
	 *
	 */
	inline size_t getMark()
	{
		return p_mark;
	}

	/*! \brief Return the memory pointer at the mark place
	 *
	 * \tparam T memory object
	 *
	 * \return memory pointer at mark place
	 *
	 */
	template<typename T> inline void * getMarkPointer(T & mem)
	{
		return mem.getPointer(p_mark);
	}

	/*! \brief Get the memory size from the mark point
	 *
	 * \tparam T memory object
	 *
	 */
	template<typename T> size_t getMarkSize(T & mem)
	{
		return (char *)mem.getPointer(un_ele) - (char *)mem.getPointer(p_mark);
	}

};

#endif /* SRC_PACK_STAT_HPP_ */
