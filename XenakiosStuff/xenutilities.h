#pragma once

template<typename Cont, typename F>
inline void remove_from_container(Cont& container, F predicate)
{
	container.erase(std::remove_if(container.begin(), container.end(), predicate), container.end());
}