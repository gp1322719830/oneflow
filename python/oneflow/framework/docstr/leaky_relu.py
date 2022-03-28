"""
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
import oneflow
from oneflow.framework.docstr.utils import add_docstr

add_docstr(
    oneflow._C.leaky_relu,
    """
    leaky_relu(x: Tensor,  alpha :Float) -> Tensor

    Applies element-wise,
    :math:`\text{LeakyReLU}(x) = \max(0, x) + \text{negative_slope} * \min(0, x)`

    See :class:`~oneflow.nn.LeakyReLU` for more details.

    """,
)
