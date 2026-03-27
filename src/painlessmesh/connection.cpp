#include "painlessmesh/connection.hpp"

namespace painlessmesh {
namespace tcp {

uint32_t lastScheduledDeletionTime = 0;
painlessmesh::buffer::temp_buffer_t shared_buffer;

}  // namespace tcp
}  // namespace painlessmesh
