from __future__ import absolute_import
import re
import oneflow.python.framework.placement_context as placement_ctx
import oneflow.python.framework.hob as hob
from oneflow.python.oneflow_export import oneflow_export
import oneflow.python.lib.core.enable_if as enable_if
import oneflow.python.eager.device_scope_stack as device_scope_stack
import oneflow

@enable_if.condition(hob.in_global_mode & hob.in_placement_scope)
def global_mode_cur_placement_scope():
    return placement_ctx.PlacementScopeStackTop()

@enable_if.condition(hob.in_normal_mode)
def normal_mode_cur_placement_scope():
    return device_scope_stack.CurrentPlacement()

@oneflow_export('placement.current_scope')
def api_current_placement_scope():
    return enable_if.unique(global_mode_cur_placement_scope, normal_mode_cur_placement_scope)()

@enable_if.condition(hob.in_global_mode
                     | (hob.in_normal_mode & hob.env_initialized & ~hob.session_initialized))
def GetFixedPlacementScope(device_tag, machine_device_ids):
    return placement_ctx.FixedPlacementScope(device_tag, machine_device_ids)

@oneflow_export('fixed_placement')
def api_fixed_placement(device_tag, machine_device_ids):
    return enable_if.unique(GetFixedPlacementScope)(device_tag, machine_device_ids)

@enable_if.condition(hob.in_global_mode
                     | (hob.in_normal_mode & hob.env_initialized & ~hob.session_initialized))
def GetDevicePriorPlacementScope(device_tag, machine_device_ids):
    return placement_ctx.DevicePriorPlacementScope(device_tag, machine_device_ids)

@oneflow_export('device_prior_placement')
def api_device_prior_placement(device_tag, machine_device_ids):
    return enable_if.unique(GetDevicePriorPlacementScope)(device_tag, machine_device_ids)

def GetDefaultMachineDeviceIds(resource):
    if resource.HasField('gpu_device_num'):
        return 'gpu', _GetGpuDefaultMachineDeviceIds(resource)
    elif resource.HasField('cpu_device_num'):
        return 'cpu', _GetCpuDefaultMachineDeviceIds(resource)
    else:
        raise NotImplementedError


def _GetGpuDefaultMachineDeviceIds(resource):
    assert resource.machine_num > 0
    assert resource.HasField('gpu_device_num')
    return ["%s:0-%s" % (m_id, resource.gpu_device_num - 1) for m_id in range(resource.machine_num)]


def _GetCpuDefaultMachineDeviceIds(resource):
    assert resource.machine_num > 0
    assert resource.HasField('cpu_device_num')
    return ["%s:0-%s" % (m_id, resource.gpu_device_num - 1) for m_id in range(resource.machine_num)]
