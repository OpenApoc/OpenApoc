#pragma once

#include <boost/date_time/date_facet.hpp>

namespace boost
{
namespace date_time
{

/*! Class that extends date_facet for our own means.
 *
 * Adds the following format strings:
 *
 *  - %E => long_day_format - Full name Ex: 1st
 */
template <class date_type, class CharT,
          class OutItrT = std::ostreambuf_iterator<CharT, std::char_traits<CharT>>>
class apoc_date_facet : public date_facet<date_type, CharT, OutItrT>
{
  public:
	explicit apoc_date_facet(::size_t a_ref = 0) : date_facet<date_type, CharT, OutItrT>(a_ref) {}

	explicit apoc_date_facet(const char_type *format_str, const input_collection_type &short_names,
	                         ::size_t ref_count = 0)
	    : date_facet<date_type, CharT, OutItrT>(format_str, short_names, ref_count)
	{
	}

	explicit apoc_date_facet(
	    const char_type *format_str, period_formatter_type per_formatter = period_formatter_type(),
	    special_values_formatter_type sv_formatter = special_values_formatter_type(),
	    date_gen_formatter_type dg_formatter = date_gen_formatter_type(), ::size_t ref_count = 0)
	    : date_facet<date_type, CharT, OutItrT>(format_str, per_formatter, sv_formatter,
	                                            dg_formatter, ref_count)
	{
	}

	static const char_type long_day_format[3];
	void long_day_names(const input_collection_type &long_names) { m_day_long_names = long_names; }
  protected:
	input_collection_type m_day_long_names;

	OutItrT do_put_tm(OutItrT next, std::ios_base &a_ios, char_type fill_char, const tm &tm_value,
	                  string_type a_format) const override
	{
		// update format string with custom names
		if (m_day_long_names.size())
		{
			boost::algorithm::replace_all(a_format, long_day_format,
			                              m_day_long_names[tm_value.tm_mday - 1]);
		}
		return date_facet<date_type, CharT, OutItrT>::do_put_tm(next, a_ios, fill_char, tm_value,
		                                                        a_format);
	}
};

template <class date_type, class CharT, class OutItrT>
const typename apoc_date_facet<date_type, CharT, OutItrT>::char_type
    apoc_date_facet<date_type, CharT, OutItrT>::long_day_format[3] = {'%', 'E'};
}

namespace gregorian
{
typedef boost::date_time::apoc_date_facet<date, char> apoc_date_facet;
}
}
