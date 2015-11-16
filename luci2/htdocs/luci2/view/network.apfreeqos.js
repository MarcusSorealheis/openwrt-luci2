L.ui.view.extend({
	title: L.tr('ApfreeQoS'),

	getIpStats: L.rpc.declare({
		object: 'luci2.network.apfreeqos',
		method: 'ip_stat',
		expect: { entries: [ ] },
	}),

	setIpRules: L.rpc.declare({
		object: 'luci2.network.apfreeqos',
		method: 'set_ip_rule'
	}),

	setVipRules: L.rpc.declare({
		object: 'luci2.network.apfreeqos',
		method: 'set_vip_rule'
	}),

	setBlRules: L.rpc.declare({
		object: 'luci2.network.apfreeqos',
		method: 'set_bl_rule'
	}),

	refreshIpStat: function() {
		var self = this;
		return $.when(
			self.getIpStats().then(function(list) {
				var ipStatTable = new L.ui.table({
					columns: [ {
						caption: L.tr('Ip'),
						key:     'ip'
					}, {
						caption: L.tr('Upload Rate (Bps)'),
						key:     'upload'
					}, {
						caption: L.tr('Download Rate (Bps)'),
						key:     'download'
					}]
				});

				ipStatTable.rows(list);
				ipStatTable.insertInto('#ip_stat_table');
				
				var data = [];
				for(var i = 0; i < list.length; i++) {
					var o = {};
					o.label = list.ip;
					o.value = list.down;
					data.push(o);
				}

				var pie = new d3pie("ip_down_rate_pie", {
						data: {
							content: data
						}
				});
				
			})
		)
	},

	fillIpRules: function() {
		var self = this;

		var m4 = new L.cbi.Map('apfreeqos', {
			caption:	L.tr('ip rule')	
		});

		var s4 = m4.section(L.cbi.GridSection, 'ip_rule', {
			caption:     L.tr('Ip Rule'),
			collabsible: true,
			addremove:   true,
			sortable:    true,
			add_caption: L.tr('Add'),
			remove_caption: L.tr('Remove'),
			readonly:   !self.options.acls.apfreeqos,
			teasers:	[ 'ip', 'netmask', 'up', 'down' ]
		});

		var device = s4.option(L.cbi.InputValue, 'ip', {
			caption:     L.tr('IP'),
			datatype:    'ip4addr',
			width:       2
		});

		var netmask = s4.option(L.cbi.InputValue, 'netmask', {
			caption:     L.tr('IPv4-Netmask'),
			datatype:    'netmask4',
			placeholder: '255.255.255.255',
			width:       2
		});

		var uprate = s4.option(L.cbi.InputValue, 'up', {
			caption:     L.tr('up rate (KBps)'),
			datatype:    'uinteger',
			placeholder: '100',
			optional:    true,
			width:       2
		});

		var downrate = s4.option(L.cbi.InputValue, 'down', {
			caption:     L.tr('down rate (KBps)'),
			datatype:    'uinterger',
			placeholder: '400',
			optional:    true,
			width:       2
		});
		
		m4.on('apply', self.setIpRules);

		m4.insertInto('#ip_rule');
	},
	
	fillVipRules: function() {
		var self = this;
		
		var m5 = new L.cbi.Map('apfreeqos', {
			caption:	L.tr('Vip rule')	
		});

		var s5 = m5.section(L.cbi.GridSection, 'vip_rule', {
			caption:     L.tr('VIP Rule'),
			collabsible: true,
			addremove:   true,
			add_caption: L.tr('Add'),
			remove_caption: L.tr('Remove'),
			readonly:    !self.options.acls.apfreeqos,
			teasers:	[ 'ip', 'netmask' ]
		});

		var vip_device = s5.option(L.cbi.InputValue, 'ip', {
			caption:     L.tr('IP'),
			datatype:    'ip4addr',
			width:       2
		});

		s5.option(L.cbi.InputValue, 'netmask', {
			caption:    	L.tr('IPv4-Netmask'),
			placeholder: 	'255.255.255.255',
			datatype:		'netmask4',
			width:      	2
		});
		
		m5.on('apply', self.setVipRules);

		m5.insertInto('#vip_rule');
	},
	
	fillBlRules: function() {
		var self = this;

		var m6 = new L.cbi.Map('apfreeqos', {
			caption:	L.tr('Blacklist rule')	
		});
		
		var s6 = m6.section(L.cbi.GridSection, 'bl_rule', {
			caption:     L.tr('Blacklist Rule'),
			anonymous:   true,
			addremove:   true,
			add_caption: L.tr('Add'),
			remove_caption: L.tr('Remove'),
			readonly:    !self.options.acls.apfreeqos,
			teasers:	[ 'ip' ]
		});

		var bl_device = s6.option(L.cbi.InputValue, 'ip', {
			caption:     L.tr('IP'),
			datatype:    'ip4addr',
			width:       2
		});
		
		m6.on('apply', self.setBlRules);

		m6.insertInto('#blacklist_rule');
	},

	execute: function() {
		var self = this;

		self.fillIpRules();
		self.fillVipRules();
		self.fillBlRules();
	
		self.repeat(self.refreshIpStat, 5000);
	}
});