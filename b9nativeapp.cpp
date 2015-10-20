/*************************************************************************************
//
//  LICENSE INFORMATION
//
//  BCreator(tm)
//  Software for the control of the 3D Printer, "B9Creator"(tm)
//
//  Copyright 2011-2012 B9Creations, LLC
//  B9Creations(tm) and B9Creator(tm) are trademarks of B9Creations, LLC
//
//  This file is part of B9Creator
//
//    B9Creator is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    B9Creator is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with B9Creator .  If not, see <http://www.gnu.org/licenses/>.
//
//  The above copyright notice and this permission notice shall be
//    included in all copies or substantial portions of the Software.
//
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
//    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
//    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
*************************************************************************************/

#include <QWidget>
#include "b9nativeapp.h"


//在任何平台上调用这个函数它是安全的。但这将只在Mac上运行有效果。
void B9NativeApp::set_smaller_text_osx(QWidget *w)
{
    return; //不使用小字体现在...
    if(w==0)return;

    //缺省情况下，所有这些尺寸属性被未设置。
    //如果有任何一个已被设置，我们就单独处理这个部件。
    if (!w->testAttribute(Qt::WA_MacMiniSize) &&
        !w->testAttribute(Qt::WA_MacSmallSize) &&
        !w->testAttribute(Qt::WA_MacNormalSize) &&
        !w->testAttribute(Qt::WA_MacVariableSize))
    {
        //这个小部件是默认的的若干文本尺寸过大类型之一吗
        if (w->inherits("QLabel") ||
            w->inherits("QLineEdit") ||
            w->inherits("QTextEdit") ||
            w->inherits("QComboBox") ||
            w->inherits("QCheckBox") ||
            w->inherits("QRadioButton") ||
            w->inherits("QAbstractItemView"))
            //其他人可以在这里添加...
        {
            // 使文本的“正常”的大小
            w->setAttribute(Qt::WA_MacMiniSize);
        }
        else if( w->inherits("QPushButton") )
            // 其他人可以在这里添加...
        {
            // 使文本的“正常”的大小
            w->setAttribute(Qt::WA_MacMiniSize);
        }
    }
}

bool B9NativeApp::event(QEvent* event)
{
    if( event->type() == QEvent::FileOpen)
    {
        //TODO使用此为Mac-OS文件关联。
        //pMain->openJob(static_cast<QFileOpenEvent*>(event)->file());
        return true;
    }
    return QApplication::event(event);
}

