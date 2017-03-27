/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
*/

#ifndef SUBSYS_VERSION_H_
#define SUBSYS_VERSION_H_

/* 说明
 * nut 不像atom,neutron有一个子系统，但为了保证统一性，为nut定义一个子系统版本号
 * 子系统的版本号，主要为了升级默认应用程序和nut的bootloader程序。
 * 子系统的版本号组成, VERSION: 为固定字符串。主要用于识别版本号。 *.*.*为默认程序版本号
 * 最后一位为bootloader的版本号。
 */

//#define  SUBSYS_VERSION         "VERSION:1.0.0.1"

/*默认程序内部更新升级到1.0.1     bootloader版本号升级到2*/
#define  SUBSYS_VERSION         "VERSION:1.0.1.2"



#endif /*SUBSYS_VERSION_H_*/


