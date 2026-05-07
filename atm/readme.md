If interviewer asks: is C++ code memory safe?
You can say:
No, because states are dynamically allocated using new but never deleted. In production C++, I would use std::unique_ptr for ownership management or singleton state instances since states are stateless.