namespace circle {

template <typename C>
struct move_aware
{
    move_aware() = default;
    move_aware(move_aware&&) noexcept { static_cast<C&>(*this).moved(); }
    move_aware& operator=(move_aware&&) { static_cast<C&>(*this).moved(); }
};
}