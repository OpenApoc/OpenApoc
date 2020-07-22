#pragma once

#include <boost/date_time.hpp>
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
class ApocDateFacet : public date_facet<date_type, CharT, OutItrT>
{
  public:
	typedef typename date_type::duration_type duration_type;
	// greg_weekday is gregorian_calendar::day_of_week_type
	typedef typename date_type::day_of_week_type day_of_week_type;
	typedef typename date_type::day_type day_type;
	typedef typename date_type::month_type month_type;
	typedef boost::date_time::period<date_type, duration_type> period_type;
	typedef std::basic_string<CharT> string_type;
	typedef CharT char_type;
	typedef boost::date_time::period_formatter<CharT> period_formatter_type;
	typedef boost::date_time::special_values_formatter<CharT> special_values_formatter_type;
	typedef std::vector<std::basic_string<CharT>> input_collection_type;
	// used for the output of the date_generators
	typedef date_generator_formatter<date_type, CharT> date_gen_formatter_type;
	typedef partial_date<date_type> partial_date_type;
	typedef nth_kday_of_month<date_type> nth_kday_type;
	typedef first_kday_of_month<date_type> first_kday_type;
	typedef last_kday_of_month<date_type> last_kday_type;
	typedef first_kday_after<date_type> kday_after_type;
	typedef first_kday_before<date_type> kday_before_type;

	explicit ApocDateFacet(::size_t a_ref = 0) : date_facet<date_type, CharT, OutItrT>(a_ref) {}

	explicit ApocDateFacet(const char_type *format_str, const input_collection_type &short_names,
	                       ::size_t ref_count = 0)
	    : date_facet<date_type, CharT, OutItrT>(format_str, short_names, ref_count)
	{
	}

	explicit ApocDateFacet(
	    const char_type *format_str, period_formatter_type per_formatter = period_formatter_type(),
	    special_values_formatter_type sv_formatter = special_values_formatter_type(),
	    date_gen_formatter_type dg_formatter = date_gen_formatter_type(), ::size_t ref_count = 0)
	    : date_facet<date_type, CharT, OutItrT>(format_str, per_formatter, sv_formatter,
	                                            dg_formatter, ref_count)
	{
	}

	static const char_type long_day_format[3];
	void longDayNames(const input_collection_type &long_names) { m_day_long_names = long_names; }

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
const typename ApocDateFacet<date_type, CharT, OutItrT>::char_type
    ApocDateFacet<date_type, CharT, OutItrT>::long_day_format[3] = {'%', 'E'};
} // namespace date_time

namespace gregorian
{
typedef boost::date_time::ApocDateFacet<date, char> apoc_date_facet;
}
} // namespace boost
