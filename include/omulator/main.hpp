#pragma once

namespace omulator {
/**
 * Omulator's main entry point. Placed in the omulator namespace for convenience.
 */
int oml_main(const int argc, const char **argv);
}  // namespace omulator

/**
 * Global entry point. Simply calls oml_main()
 */
int main(const int argc, const char **argv);
